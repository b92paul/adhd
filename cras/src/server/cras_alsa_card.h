/* Copyright 2012 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_SERVER_CRAS_ALSA_CARD_H_
#define CRAS_SRC_SERVER_CRAS_ALSA_CARD_H_

#include "cras_types.h"

/* cras_alsa_card represents an alsa sound card.  It adds all the devices for
 * this card to the system when it is created, and removes them when it is
 * destroyed.  It will create an alsa_mixer object that can control the volume
 * and mute settings for the card.
 */

struct cras_alsa_card;
struct cras_device_blocklist;

/* Creates a cras_alsa_card instance for the given alsa device.  Enumerates the
 * devices for the card and adds them to the system as possible playback or
 * capture endpoints.
 * Args:
 *    card_info - Contains the card index, type, and priority.
 *    device_config_dir - The directory of device configs which contains the
 *                        volume curves.
 *    blocklist - List of devices that should be ignored.
 *    ucm_suffix - The ucm config name is formed as <card-name>.<suffix>
 * Returns:
 *    A pointer to the newly created cras_alsa_card which must later be freed
 *    by calling cras_alsa_card_destroy or NULL on error.
 */
struct cras_alsa_card* cras_alsa_card_create(
    struct cras_alsa_card_info* info,
    const char* device_config_dir,
    struct cras_device_blocklist* blocklist,
    const char* ucm_suffix);

/* Destroys a cras_alsa_card that was returned from cras_alsa_card_create.
 * Args:
 *    alsa_card - The cras_alsa_card pointer returned from
 *        cras_alsa_card_create.
 */
void cras_alsa_card_destroy(struct cras_alsa_card* alsa_card);

/* Returns the alsa card index for the given card.
 * Args:
 *    alsa_card - The cras_alsa_card pointer returned from
 *        cras_alsa_card_create.
 */
size_t cras_alsa_card_get_index(const struct cras_alsa_card* alsa_card);

/* Returns the alsa card type for the given card.
 * Args:
 *    alsa_card - The cras_alsa_card pointer returned from
 *        cras_alsa_card_create.
 */
enum CRAS_ALSA_CARD_TYPE cras_alsa_card_get_type(
    const struct cras_alsa_card* alsa_card);

#endif  // CRAS_SRC_SERVER_CRAS_ALSA_CARD_H_
