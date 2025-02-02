/* Copyright 2012 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_SERVER_CRAS_EMPTY_IODEV_H_
#define CRAS_SRC_SERVER_CRAS_EMPTY_IODEV_H_

#include "cras_types.h"

struct cras_iodev;

/* Initializes an empty iodev.  Empty iodevs are used when there are no other
 * iodevs available.  They give the attached streams a temporary place to live
 * until a new iodev becomes available.
 * Args:
 *    direciton - input or output.
 *    node_type - the default node type.
 * Returns:
 *    A pointer to the newly created iodev if successful, NULL otherwise.
 */
struct cras_iodev* empty_iodev_create(enum CRAS_STREAM_DIRECTION direction,
                                      enum CRAS_NODE_TYPE node_type);

// Destroys an empty_iodev created with empty_iodev_create.
void empty_iodev_destroy(struct cras_iodev* iodev);

#endif  // CRAS_EMPTY_IO_H_
