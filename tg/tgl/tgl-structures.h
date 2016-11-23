/* 
    This file is part of tgl-library

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright Vitaly Valtman 2013-2015
*/
#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

#include <assert.h>
#include "tgl-layout.h"
#include "tgl-fetch.h"
#include "tgl.h"

char *tgls_default_create_print_name (struct tgl_state *TLS, tgl_peer_id_t id, const char *a1, const char *a2, const char *a3, const char *a4);


void tgls_free_user (struct tgl_state *TLS, struct tgl_user *U);
void tgls_free_chat (struct tgl_state *TLS, struct tgl_chat *U);
void tgls_free_photo (struct tgl_state *TLS, struct tgl_photo *P);
void tgls_free_message (struct tgl_state *TLS, struct tgl_message *M);
void tgls_free_bot_info (struct tgl_state *TLS, struct tgl_bot_info *B);
void tgls_clear_message (struct tgl_state *TLS, struct tgl_message *M);

struct tgl_message *tglm_message_alloc (struct tgl_state *TLS, tgl_message_id_t *id);
void tglm_message_insert_tree (struct tgl_state *TLS, struct tgl_message *M);
void tglm_update_message_id (struct tgl_state *TLS, struct tgl_message *M, long long id);
void tglm_message_insert (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_insert_unsent (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_remove_unsent (struct tgl_state *TLS, struct tgl_message *M);
void tglm_send_all_unsent (struct tgl_state *TLS);
void tglm_message_remove_tree (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_add_peer (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_del_peer (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_del_use (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_add_use (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_del_temp_id (struct tgl_state *TLS, struct tgl_message *M);
void tglm_message_del_random_id (struct tgl_state *TLS, struct tgl_message *M);

void tglp_peer_insert_name (struct tgl_state *TLS, tgl_peer_t *P);
void tglp_peer_delete_name (struct tgl_state *TLS, tgl_peer_t *P);
void tglp_insert_encrypted_chat (struct tgl_state *TLS, tgl_peer_t *P);
void tglp_insert_user (struct tgl_state *TLS, tgl_peer_t *P);
void tglp_insert_chat (struct tgl_state *TLS, tgl_peer_t *P);
void tglp_insert_channel (struct tgl_state *TLS, tgl_peer_t *P);
//enum tgl_typing_status tglf_fetch_typing_buf (void);
void tgls_messages_mark_read (struct tgl_state *TLS, struct tgl_message *M, int out, int seq);

//void tgls_insert_random2local (struct tgl_state *TLS, long long random_id, tgl_message_id_t *local_id);
//void tgls_insert_temp2local (struct tgl_state *TLS, int temp_id, tgl_message_id_t *local_id);
tgl_message_id_t *tgls_get_local_by_random (struct tgl_state *TLS, long long random_id);
tgl_message_id_t *tgls_get_local_by_temp (struct tgl_state *TLS, int temp_id);


void tgls_message_change_temp_id (struct tgl_state *TLS, struct tgl_message *M, int temp_id);
void tgls_message_change_random_id (struct tgl_state *TLS, struct tgl_message *M, long long random_id);


void tgl_photo_insert (struct tgl_state *TLS, struct tgl_photo *P);
struct tgl_photo *tgl_photo_get (struct tgl_state *TLS, long long id);
struct tgl_document *tgl_document_get (struct tgl_state *TLS, long long id);
void tgl_document_insert (struct tgl_state *TLS, struct tgl_document *P);
struct tgl_webpage *tgl_webpage_get (struct tgl_state *TLS, long long id);
void tgl_webpage_insert (struct tgl_state *TLS, struct tgl_webpage *P);

tgl_message_id_t tgl_convert_temp_msg_id (struct tgl_state *TLS, tgl_message_id_t msg_id);

static inline tgl_peer_id_t tgl_msg_id_to_peer_id (tgl_message_id_t msg_id) {
  tgl_peer_id_t id;
  id.peer_type = msg_id.peer_type;
  id.peer_id = msg_id.peer_id;
  id.access_hash = msg_id.access_hash;
  return id;
}

static inline tgl_message_id_t tgl_peer_id_to_msg_id (tgl_peer_id_t peer_id, long long msg_id) {
  tgl_message_id_t id;
  id.peer_type = peer_id.peer_type;
  id.peer_id = peer_id.peer_id;
  id.access_hash = peer_id.access_hash;
  id.id = msg_id;
  return id;
}

static inline tgl_message_id_t tgl_peer_id_to_random_msg_id (tgl_peer_id_t peer_id) {
  long long id;
  tglt_secure_random (&id, 8);
  return tgl_peer_id_to_msg_id (peer_id, id);
}
#endif
