// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdio.h>
#include <unistd.h>

extern "C" {
#include "cras/src/server/audio_thread.h"
#include "cras/src/server/cras_bt_log.h"
#include "cras/src/server/cras_rclient.h"
#include "cras/src/server/cras_rstream.h"
#include "cras/src/server/cras_system_state.h"
#include "cras_messages.h"

// Access to data structures and static functions.
#include "cras/src/server/cras_capture_rclient.c"
#include "cras/src/server/cras_rclient_util.c"
}
static unsigned int cras_make_fd_nonblocking_called;
static unsigned int cras_observer_remove_called;
static int stream_list_add_called;
static int stream_list_add_return;
static unsigned int stream_list_rm_called;
static struct cras_audio_shm mock_shm;
static struct cras_rstream mock_rstream;

void ResetStubData() {
  cras_make_fd_nonblocking_called = 0;
  cras_observer_remove_called = 0;
  stream_list_add_called = 0;
  stream_list_add_return = 0;
  stream_list_rm_called = 0;
}

namespace {

TEST(RClientSuite, CreateSendMessage) {
  struct cras_rclient* rclient;
  int rc;
  struct cras_client_connected msg;
  int pipe_fds[2];

  ResetStubData();

  rc = pipe(pipe_fds);
  ASSERT_EQ(0, rc);

  rclient = cras_capture_rclient_create(pipe_fds[1], 800);
  ASSERT_NE((void*)NULL, rclient);
  EXPECT_EQ(800, rclient->id);

  rc = read(pipe_fds[0], &msg, sizeof(msg));
  EXPECT_EQ(sizeof(msg), rc);
  EXPECT_EQ(CRAS_CLIENT_CONNECTED, msg.header.id);

  rclient->ops->destroy(rclient);
  EXPECT_EQ(1, cras_observer_remove_called);
  close(pipe_fds[0]);
  close(pipe_fds[1]);
}

class CCRMessageSuite : public testing::Test {
 protected:
  virtual void SetUp() {
    int rc;
    struct cras_client_connected msg;

    rc = pipe(pipe_fds_);
    if (rc < 0) {
      return;
    }

    rclient_ = cras_capture_rclient_create(pipe_fds_[1], 1);
    rc = read(pipe_fds_[0], &msg, sizeof(msg));
    if (rc < 0) {
      return;
    }

    fmt = {
        .format = SND_PCM_FORMAT_S16_LE,
        .frame_rate = 48000,
        .num_channels = 2,
    };
    cras_audio_format_set_default_channel_layout(&fmt);
    ResetStubData();
  }

  virtual void TearDown() {
    rclient_->ops->destroy(rclient_);
    close(pipe_fds_[0]);
    close(pipe_fds_[1]);
  }

  struct cras_rclient* rclient_;
  struct cras_audio_format fmt;
  int pipe_fds_[2];
  int fd_;
};

TEST_F(CCRMessageSuite, StreamConnectMessage) {
  struct cras_client_stream_connected out_msg;
  int rc;

  struct cras_connect_message msg;
  cras_stream_id_t stream_id = 0x10002;
  cras_fill_connect_message(&msg, CRAS_STREAM_INPUT, stream_id,
                            CRAS_STREAM_TYPE_DEFAULT, CRAS_CLIENT_TYPE_UNKNOWN,
                            480, 240, /*flags=*/0, /*effects=*/0, fmt,
                            NO_DEVICE);
  ASSERT_EQ(stream_id, msg.stream_id);

  fd_ = 100;
  rclient_->ops->handle_message_from_client(rclient_, &msg.header, &fd_, 1);
  EXPECT_EQ(1, cras_make_fd_nonblocking_called);
  EXPECT_EQ(1, stream_list_add_called);
  EXPECT_EQ(0, stream_list_rm_called);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(stream_id, out_msg.stream_id);
}

TEST_F(CCRMessageSuite, StreamConnectMessageInvalidDirection) {
  struct cras_client_stream_connected out_msg;
  int rc;

  struct cras_connect_message msg;
  cras_stream_id_t stream_id = 0x10002;

  for (int i = 0; i < CRAS_NUM_DIRECTIONS; i++) {
    const auto dir = static_cast<CRAS_STREAM_DIRECTION>(i);
    if (dir == CRAS_STREAM_INPUT) {
      continue;
    }
    cras_fill_connect_message(&msg, dir, stream_id, CRAS_STREAM_TYPE_DEFAULT,
                              CRAS_CLIENT_TYPE_UNKNOWN, 480, 240, /*flags=*/0,
                              /*effects=*/0, fmt, NO_DEVICE);
    ASSERT_EQ(stream_id, msg.stream_id);

    fd_ = 100;
    rc = rclient_->ops->handle_message_from_client(rclient_, &msg.header, &fd_,
                                                   1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0, cras_make_fd_nonblocking_called);
    EXPECT_EQ(0, stream_list_add_called);
    EXPECT_EQ(0, stream_list_rm_called);

    rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
    EXPECT_EQ(sizeof(out_msg), rc);
    EXPECT_EQ(-EINVAL, out_msg.err);
    EXPECT_EQ(stream_id, out_msg.stream_id);
  }
}

TEST_F(CCRMessageSuite, StreamConnectMessageInvalidClientId) {
  struct cras_client_stream_connected out_msg;
  int rc;

  struct cras_connect_message msg;
  cras_stream_id_t stream_id = 0x20002;  // stream_id with invalid client_id
  cras_fill_connect_message(&msg, CRAS_STREAM_INPUT, stream_id,
                            CRAS_STREAM_TYPE_DEFAULT, CRAS_CLIENT_TYPE_UNKNOWN,
                            480, 240, /*flags=*/0, /*effects=*/0, fmt,
                            NO_DEVICE);
  ASSERT_EQ(stream_id, msg.stream_id);

  fd_ = 100;
  rc =
      rclient_->ops->handle_message_from_client(rclient_, &msg.header, &fd_, 1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, cras_make_fd_nonblocking_called);
  EXPECT_EQ(0, stream_list_add_called);
  EXPECT_EQ(0, stream_list_rm_called);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(-EINVAL, out_msg.err);
  EXPECT_EQ(stream_id, out_msg.stream_id);
}

TEST_F(CCRMessageSuite, StreamDisconnectMessage) {
  struct cras_disconnect_stream_message msg;
  cras_stream_id_t stream_id = 0x10002;
  cras_fill_disconnect_stream_message(&msg, stream_id);

  rclient_->ops->handle_message_from_client(rclient_, &msg.header, NULL, 0);
  EXPECT_EQ(0, stream_list_add_called);
  EXPECT_EQ(1, stream_list_rm_called);
}

TEST_F(CCRMessageSuite, StreamDisconnectMessageInvalidClientId) {
  struct cras_disconnect_stream_message msg;
  cras_stream_id_t stream_id = 0x20002;  // stream_id with invalid client_id
  cras_fill_disconnect_stream_message(&msg, stream_id);

  rclient_->ops->handle_message_from_client(rclient_, &msg.header, NULL, 0);
  EXPECT_EQ(0, stream_list_add_called);
  EXPECT_EQ(0, stream_list_rm_called);
}
}  // namespace

// stubs
extern "C" {

struct stream_list* cras_iodev_list_get_stream_list() {
  return NULL;
}

int cras_iodev_list_set_aec_ref(unsigned int stream_id, unsigned int dev_idx) {
  return 0;
}

int cras_make_fd_nonblocking(int fd) {
  cras_make_fd_nonblocking_called++;
  return 0;
}

void cras_observer_remove(struct cras_observer_client* client) {
  cras_observer_remove_called++;
}

unsigned int cras_rstream_get_effects(const struct cras_rstream* stream) {
  return 0;
}

int cras_send_with_fds(int sockfd,
                       const void* buf,
                       size_t len,
                       int* fd,
                       unsigned int num_fds) {
  return write(sockfd, buf, len);
}

key_t cras_sys_state_shm_fd() {
  return 1;
}

void cras_system_set_suspended(int suspended) {}

int stream_list_rm_all_client_streams(struct stream_list* list,
                                      struct cras_rclient* rclient) {
  return 0;
}

int stream_list_rm(struct stream_list* list, cras_stream_id_t id) {
  stream_list_rm_called++;
  return 0;
}

int stream_list_add(struct stream_list* list,
                    struct cras_rstream_config* config,
                    struct cras_rstream** stream) {
  int ret;

  *stream = &mock_rstream;

  stream_list_add_called++;
  ret = stream_list_add_return;
  if (ret) {
    stream_list_add_return = -EINVAL;
  }

  mock_rstream.shm = &mock_shm;
  mock_rstream.direction = config->direction;
  mock_rstream.stream_id = config->stream_id;

  return ret;
}

bool cras_audio_format_valid(const struct cras_audio_format* fmt) {
  return true;
}

void detect_rtc_stream_pair(struct stream_list* list,
                            struct cras_rstream* stream) {
  return;
}
int cras_server_metrics_stream_connect_failure(
    enum CRAS_STREAM_CONNECT_ERROR code) {
  return 0;
}

}  // extern "C"
