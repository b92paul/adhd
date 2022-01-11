// Copyright 2022 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include "cras_types.h"

extern "C" {
#include "cras_hfp_manager.h"
#include "cras_iodev.h"
}

static struct cras_hfp* hfp_pcm_iodev_create_hfp_val;
static size_t hfp_pcm_iodev_create_called;
static size_t hfp_pcm_iodev_destroy_called;

void ResetStubData() {
  hfp_pcm_iodev_create_hfp_val = NULL;
  hfp_pcm_iodev_create_called = 0;
  hfp_pcm_iodev_destroy_called = 0;
}

namespace {

class HfpManagerTestSuite : public testing::Test {
 protected:
  virtual void SetUp() { ResetStubData(); }

  virtual void TearDown() {}
};

TEST_F(HfpManagerTestSuite, CreateDestroy) {
  struct cras_hfp* hfp = cras_floss_hfp_create(NULL, "addr");
  ASSERT_NE(hfp, (struct cras_hfp*)NULL);
  EXPECT_EQ(hfp, hfp_pcm_iodev_create_hfp_val);
  EXPECT_EQ(hfp_pcm_iodev_create_called, 2);

  // Expect another call to hfp create returns null.
  struct cras_hfp* expect_null = cras_floss_hfp_create(NULL, "addr2");
  EXPECT_EQ((void*)NULL, expect_null);

  cras_floss_hfp_destroy(hfp);
  EXPECT_EQ(hfp_pcm_iodev_destroy_called, 2);
}

}  // namespace

extern "C" {

struct cras_iodev* hfp_pcm_iodev_create(struct cras_hfp* hfp,
                                        enum CRAS_STREAM_DIRECTION dir) {
  hfp_pcm_iodev_create_hfp_val = hfp;
  hfp_pcm_iodev_create_called++;
  return reinterpret_cast<struct cras_iodev*>(0x123);
}

void hfp_pcm_iodev_destroy(struct cras_iodev* iodev) {
  hfp_pcm_iodev_destroy_called++;
  return;
}

}  // extern "C"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}