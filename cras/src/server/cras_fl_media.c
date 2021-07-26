/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "cras_a2dp_manager.h"

#define BT_SERVICE_NAME "org.chromium.bluetooth"
/* Object path is of the form BT_OBJECT_BASE + hci + BT_OBJECT_MEDIA */
#define BT_OBJECT_BASE "/org/chromium/bluetooth/hci"
#define BT_OBJECT_MEDIA "/media"
#define BT_MEDIA_INTERFACE "org.chromium.bluetooth.BluetoothMedia"

#define BT_MEDIA_CALLBACK_INTERFACE                                            \
	"org.chromium.bluetooth.BluetoothMediaCallback"

#define CRAS_BT_MEDIA_OBJECT_PATH "/org/chromium/cras/bluetooth/media"
#define BT_MEDIA_OBJECT_PATH_SIZE_MAX 128

struct fl_media {
	unsigned int hci;
	char obj_path[BT_MEDIA_OBJECT_PATH_SIZE_MAX];
	DBusConnection *conn;
	struct cras_a2dp *a2dp;
};

static struct fl_media *active_fm = NULL;

struct fl_media *fl_media_create(int hci)
{
	struct fl_media *fm = (struct fl_media *)calloc(1, sizeof(*fm));

	if (fm == NULL)
		return NULL;
	fm->hci = hci;
	snprintf(fm->obj_path, BT_MEDIA_OBJECT_PATH_SIZE_MAX, "%s%d%s",
		 BT_OBJECT_BASE, hci, BT_OBJECT_MEDIA);
	return fm;
}

/* helper to extract a single argument from a DBus message. */
static int get_single_arg(DBusMessage *message, int dbus_type, void *arg)
{
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error, dbus_type, arg,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING, "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return 0;
}

static void floss_on_initialize(DBusPendingCall *pending_call, void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "Initialize returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

static int floss_media_init(DBusConnection *conn, const struct fl_media *fm)
{
	DBusMessage *method_call;
	DBusPendingCall *pending_call;

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE, "Initialize");
	if (!method_call)
		return -ENOMEM;

	pending_call = NULL;
	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call, floss_on_initialize,
					  conn, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}
	return 0;
}

int floss_media_a2dp_set_active_device(struct fl_media *fm, const char *addr)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;

	syslog(LOG_DEBUG, "floss_media_set_active_device");

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE,
					     "SetActiveDevice");
	if (!method_call)
		return -ENOMEM;

	if (!dbus_message_append_args(method_call, DBUS_TYPE_STRING, &addr,
				      DBUS_TYPE_INVALID)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_error_init(&dbus_error);
	reply = dbus_connection_send_with_reply_and_block(
		active_fm->conn, method_call, DBUS_TIMEOUT_USE_DEFAULT,
		&dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to send SetActiveDevice %s: %s", addr,
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "SetActiveDevice returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}
	return 0;
}

int floss_media_a2dp_set_audio_config(struct fl_media *fm, unsigned int rate,
				      unsigned int bps, unsigned int channels)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;
	dbus_uint32_t sample_rate = rate;
	dbus_uint32_t bits_per_sample = bps;
	dbus_uint32_t channel_mode = channels;

	syslog(LOG_DEBUG, "floss_media_a2dp_set_audio_config");

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE,
					     "SetAudioConfig");
	if (!method_call)
		return -ENOMEM;

	if (!dbus_message_append_args(method_call, DBUS_TYPE_INT32,
				      &sample_rate, DBUS_TYPE_INVALID)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}
	if (!dbus_message_append_args(method_call, DBUS_TYPE_INT32,
				      &bits_per_sample, DBUS_TYPE_INVALID)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}
	if (!dbus_message_append_args(method_call, DBUS_TYPE_INT32,
				      &channel_mode, DBUS_TYPE_INVALID)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_error_init(&dbus_error);
	reply = dbus_connection_send_with_reply_and_block(
		fm->conn, method_call, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to send SetAudioConfig: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "SetAudioConfig returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}
	return 0;
}

int floss_media_a2dp_start_audio_request(struct fl_media *fm)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;

	syslog(LOG_DEBUG, "floss_media_a2dp_start_audio_request");

	if (!fm) {
		syslog(LOG_WARNING, "%s: Floss media not started", __func__);
		return -EINVAL;
	}

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE,
					     "StartAudioRequest");
	if (!method_call)
		return -ENOMEM;

	dbus_error_init(&dbus_error);
	reply = dbus_connection_send_with_reply_and_block(
		fm->conn, method_call, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to send StartAudioRequest: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "StartAudioRequest returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}
	return 0;
}

int floss_media_a2dp_stop_audio_request(struct fl_media *fm)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;

	syslog(LOG_DEBUG, "floss_media_a2dp_stop_audio_request");

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE,
					     "StopAudioRequest");
	if (!method_call)
		return -ENOMEM;

	dbus_error_init(&dbus_error);

	reply = dbus_connection_send_with_reply_and_block(
		fm->conn, method_call, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to send StopAudioRequest: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "StopAudioRequest returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}
	return 0;
}

static void floss_on_register_callback(DBusPendingCall *pending_call,
				       void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "RegisterCallback returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}
	dbus_message_unref(reply);
}

static int floss_media_register_callback(DBusConnection *conn,
					 const struct fl_media *fm)
{
	const char *bt_media_object_path = CRAS_BT_MEDIA_OBJECT_PATH;
	DBusMessage *method_call;
	DBusPendingCall *pending_call;

	method_call =
		dbus_message_new_method_call(BT_SERVICE_NAME, fm->obj_path,
					     BT_MEDIA_INTERFACE,
					     "RegisterCallback");
	if (!method_call)
		return -ENOMEM;

	if (!dbus_message_append_args(method_call, DBUS_TYPE_OBJECT_PATH,
				      &bt_media_object_path,
				      DBUS_TYPE_INVALID)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	pending_call = NULL;
	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(
		    pending_call, floss_on_register_callback, conn, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}

static DBusHandlerResult
handle_bt_media_callback(DBusConnection *conn, DBusMessage *message, void *arg)
{
	int rc;
	char *addr = NULL;
	DBusError dbus_error;
	dbus_int32_t sample_rate, bits_per_sample, channel_mode;

	syslog(LOG_DEBUG, "Bt Media callback message: %s %s %s",
	       dbus_message_get_path(message),
	       dbus_message_get_interface(message),
	       dbus_message_get_member(message));

	if (dbus_message_is_method_call(message, BT_MEDIA_CALLBACK_INTERFACE,
					"OnBluetoothAudioDeviceAdded")) {
		dbus_error_init(&dbus_error);
		if (!dbus_message_get_args(
			    message, &dbus_error, DBUS_TYPE_STRING, &addr,
			    DBUS_TYPE_INT32, &sample_rate, DBUS_TYPE_INT32,
			    &bits_per_sample, DBUS_TYPE_INT32, &channel_mode,
			    DBUS_TYPE_INVALID)) {
			syslog(LOG_WARNING,
			       "Bad OnBluetoothAudioDeviceAdded method received: %s",
			       dbus_error.message);
			dbus_error_free(&dbus_error);
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		syslog(LOG_DEBUG, "OnBluetoothAudioDeviceAdded %s %d %d %d",
		       addr, sample_rate, bits_per_sample, channel_mode);
		if (!active_fm) {
			syslog(LOG_WARNING, "Floss media object not ready");
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		if (active_fm->a2dp) {
			syslog(LOG_WARNING,
			       "Multiple A2DP devices added, override the older");
			cras_floss_a2dp_destroy(active_fm->a2dp);
		}
		active_fm->a2dp =
			cras_floss_a2dp_create(active_fm, addr, sample_rate,
					       bits_per_sample, channel_mode);

		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (dbus_message_is_method_call(
			   message, BT_MEDIA_CALLBACK_INTERFACE,
			   "OnBluetoothAudioDeviceRemoved")) {
		rc = get_single_arg(message, DBUS_TYPE_STRING, &addr);
		if (rc) {
			syslog(LOG_ERR,
			       "Failed to get addr from OnBluetoothAudioDeviceRemoved");
			return rc;
		}

		syslog(LOG_DEBUG, "OnBluetoothAudioDeviceRemoved %s", addr);
		if (active_fm && active_fm->a2dp) {
			cras_floss_a2dp_destroy(active_fm->a2dp);
			active_fm->a2dp = NULL;
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* When we're notified about Floss media interface is ready. */
int floss_media_start(DBusConnection *conn, unsigned int hci)
{
	static const DBusObjectPathVTable control_vtable = {
		.message_function = handle_bt_media_callback,
	};
	DBusError dbus_error;

	// Register the callbacks to dbus daemon.
	dbus_error_init(&dbus_error);
	if (!dbus_connection_register_object_path(
		    conn, CRAS_BT_MEDIA_OBJECT_PATH, &control_vtable,
		    &dbus_error)) {
		syslog(LOG_ERR, "Couldn't register CRAS control: %s: %s",
		       CRAS_BT_MEDIA_OBJECT_PATH, dbus_error.message);
		dbus_error_free(&dbus_error);
		return -1;
	}

	/* Try to be cautious if Floss media gets the state wrong. */
	if (active_fm) {
		syslog(LOG_WARNING,
		       "Floss media %s already started, overriding by hci %u",
		       active_fm->obj_path, hci);
		free(active_fm);
	}

	active_fm = fl_media_create(hci);
	if (active_fm == NULL)
		return -ENOMEM;
	active_fm->conn = conn;

	syslog(LOG_DEBUG, "floss_media_start");
	floss_media_register_callback(conn, active_fm);
	floss_media_init(conn, active_fm);
	return 0;
}

int floss_media_stop(DBusConnection *conn)
{
	if (!dbus_connection_unregister_object_path(conn,
						    CRAS_BT_MEDIA_OBJECT_PATH))
		syslog(LOG_WARNING, "Couldn't unregister BT media obj path");

	/* Clean up iodev when BT forced to stop. */
	if (active_fm) {
		if (active_fm->a2dp)
			cras_floss_a2dp_destroy(active_fm->a2dp);
		free(active_fm);
		active_fm = NULL;
	}
	return 0;
}