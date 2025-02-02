/* Copyright 2012 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A remote client to the server.
 */
#ifndef CRAS_SRC_SERVER_CRAS_RCLIENT_H_
#define CRAS_SRC_SERVER_CRAS_RCLIENT_H_

#include "cras_types.h"

struct cras_client_message;
struct cras_message;
struct cras_server_message;

// An attached client.
struct cras_rclient {
  struct cras_observer_client* observer;
  // The id of the client.
  size_t id;
  // Connection for client communication.
  int fd;
  // cras_rclient_ops for the cras_rclient.
  const struct cras_rclient_ops* ops;
  // Bit mask for supported stream directions.
  int supported_directions;
  // Client type of this rclient. If this is set to value other
  // than CRAS_CLIENT_TYPE_UNKNOWN, rclient will overwrite incoming
  // messages' client type.
  enum CRAS_CLIENT_TYPE client_type;
};

// Operations for cras_rclient.
struct cras_rclient_ops {
  // Entry point for handling a message from the
  // corresponded client.
  int (*handle_message_from_client)(struct cras_rclient*,
                                    const struct cras_server_message*,
                                    int* fds,
                                    unsigned int num_fds);
  // Method for sending message to the corresponded
  // client.
  int (*send_message_to_client)(const struct cras_rclient*,
                                const struct cras_client_message*,
                                int* fds,
                                unsigned int num_fds);
  // Method to destroy and free the cras_rclient.
  void (*destroy)(struct cras_rclient*);
};

/* Creates an rclient structure.
 * Args:
 *    fd - The file descriptor used for communication with the client.
 *    id - Unique identifier for this client.
 *    conn_type - Client connection type.
 * Returns:
 *    A pointer to the newly created rclient on success, NULL on failure.
 */
struct cras_rclient* cras_rclient_create(int fd,
                                         size_t id,
                                         enum CRAS_CONNECTION_TYPE conn_type);

/* Destroys an rclient created with "cras_rclient_create".
 * Args:
 *    client - The client to destroy.
 */
void cras_rclient_destroy(struct cras_rclient* client);

/* Handles a received buffer from the client.
 * Args:
 *    client - The client that received this message.
 *    buf - The raw byte buffer the client sent. It should contain a valid
 *      cras_server_message.
 *    buf_len - The length of |buf|.
 *    fds - Array of valid file descriptors sent by the remote client.
 *    num_fds - Length of |fds|.
 * Returns:
 *    0 on success, otherwise a negative error code.
 */
int cras_rclient_buffer_from_client(struct cras_rclient* client,
                                    const uint8_t* buf,
                                    size_t buf_len,
                                    int* fds,
                                    int num_fds);

/* Sends a message to the client.
 * Args:
 *    client - The client to send the message to.
 *    msg - The message to send.
 *    fds - Array of file descriptors or null
 *    num_fds - Number of entries in the fds array.
 * Returns:
 *    number of bytes written on success, otherwise a negative error code.
 */
int cras_rclient_send_message(const struct cras_rclient* client,
                              const struct cras_client_message* msg,
                              int* fds,
                              unsigned int num_fds);

#endif  // CRAS_SRC_SERVER_CRAS_RCLIENT_H_
