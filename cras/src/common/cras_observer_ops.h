/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_COMMON_CRAS_OBSERVER_OPS_H_
#define CRAS_SRC_COMMON_CRAS_OBSERVER_OPS_H_

#include "cras_types.h"

/* Observation of CRAS state.
 * Unless otherwise specified, all notifications only contain the data value
 * reflecting the current state: it is possible that multiple notifications
 * are queued within CRAS before being sent to the client.
 */
struct cras_observer_ops {
  // System output volume changed.
  void (*output_volume_changed)(void* context, int32_t volume);
  // System output mute changed.
  void (*output_mute_changed)(void* context,
                              int muted,
                              int user_muted,
                              int mute_locked);
  // System input/capture gain changed.
  void (*capture_gain_changed)(void* context, int32_t gain);
  // System input/capture mute changed.
  void (*capture_mute_changed)(void* context, int muted, int mute_locked);
  // Device or node topology changed.
  void (*nodes_changed)(void* context);
  /* Active node changed. A notification is sent for every change.
   * When there is no active node, node_id is 0. */
  void (*active_node_changed)(void* context,
                              enum CRAS_STREAM_DIRECTION dir,
                              cras_node_id_t node_id);
  // Output node volume changed.
  void (*output_node_volume_changed)(void* context,
                                     cras_node_id_t node_id,
                                     int32_t volume);
  // Node left/right swapped state change.
  void (*node_left_right_swapped_changed)(void* context,
                                          cras_node_id_t node_id,
                                          int swapped);
  // Input gain changed.
  void (*input_node_gain_changed)(void* context,
                                  cras_node_id_t node_id,
                                  int32_t gain);
  // Suspend state changed.
  void (*suspend_changed)(void* context, int suspended);
  // Number of active streams changed.
  void (*num_active_streams_changed)(void* context,
                                     enum CRAS_STREAM_DIRECTION dir,
                                     uint32_t num_active_streams);
  // Number of non-chrome output streams changed.
  void (*num_non_chrome_output_streams_changed)(
      void* context,
      uint32_t num_non_chrome_output_streams);
  // Number of input streams with permission changed.
  void (*num_input_streams_with_permission_changed)(
      void* context,
      uint32_t num_input_streams[CRAS_NUM_CLIENT_TYPE]);
  // Hotword triggered.
  void (*hotword_triggered)(void* context, int64_t tv_sec, int64_t tv_nsec);
  /* State regarding whether non-empty audio is being played/captured has
   * changed. */
  void (*non_empty_audio_state_changed)(void* context, int non_empty);
  // Bluetooth headset battery level changed.
  void (*bt_battery_changed)(void* context,
                             const char* address,
                             uint32_t level);
  // Severe audio underrun event
  void (*severe_underrun)(void* context);
  // Audio underrun event
  void (*underrun)(void* context);
  // General Survey trigger event
  void (*general_survey)(void* context,
                         enum CRAS_STREAM_TYPE stream_type,
                         enum CRAS_CLIENT_TYPE client_type,
                         const char* node_type_pair);
  // Speech detected while on mute
  void (*speak_on_mute_detected)(void* context);
};

#endif  // CRAS_SRC_COMMON_CRAS_OBSERVER_OPS_H_
