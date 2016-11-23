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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "crypto/bn.h"

#include "tgl-binlog.h"
#include "mtproto-common.h"
//#include "net.h"
#include "mtproto-client.h"
#include "mtproto-utils.h"

#include "tgl.h"
#include "auto.h"
#include "auto/auto-types.h"
#include "auto/auto-skip.h"
#include "auto/auto-store-ds.h"
#include "auto/auto-fetch-ds.h"
#include "auto/auto-free-ds.h"

#include "tgl-structures.h"
#include "tgl-methods-in.h"

#include "crypto/sha.h"

static int mystreq1 (const char *a, const char *b, int l) {
  if ((int)strlen (a) != l) { return 1; }
  return memcmp (a, b, l);
}

void bl_do_dc_option (struct tgl_state *TLS, int flags, int id, const char *name, int l1, const char *ip, int l2, int port) /* {{{ */ {
  struct tgl_dc *DC = TLS->DC_list[id];

  if (DC) {
    struct tgl_dc_option *O = DC->options[flags & 3];
    while (O) {
      if (!strncmp (O->ip, ip, l2)) {
        return;
      }
      O = O->next;
    }
  }
  
  vlogprintf (E_NOTICE, "DC%d '%.*s' update: %.*s:%d\n", 
    id, 
    l1, name,
    l2, ip,
    port
  );

  tglmp_alloc_dc (TLS,
    flags,
    id,
    tstrndup (ip, l2),
    port
  );
}
/* }}} */

void bl_do_set_working_dc (struct tgl_state *TLS, int num) /* {{{ */ {
  assert (num > 0 && num <= MAX_DC_ID);
  TLS->DC_working = TLS->DC_list[num];
  TLS->dc_working_num = num;
}
/* }}} */

void bl_do_dc_signed (struct tgl_state *TLS, int num) /* {{{ */ {
  assert (num > 0 && num <= MAX_DC_ID);
  assert (TLS->DC_list[num]);
  TLS->DC_list[num]->flags |= TGLDCF_LOGGED_IN;
}
/* }}} */

void bl_do_set_auth_key (struct tgl_state *TLS, int num, unsigned char *buf) /* {{{ */ {
  assert (num > 0 && num <= MAX_DC_ID);
  assert (TLS->DC_list[num]);

  memcpy (TLS->DC_list[num]->auth_key, buf, 256);
  
  static unsigned char sha1_buffer[20];
  TGLC_sha1 ((void *)TLS->DC_list[num]->auth_key, 256, sha1_buffer);
  TLS->DC_list[num]->auth_key_id = *(long long *)(sha1_buffer + 12);

  TLS->DC_list[num]->flags |= TGLDCF_AUTHORIZED;
}
/* }}} */

void bl_do_set_our_id (struct tgl_state *TLS, tgl_peer_id_t id) /* {{{ */ {
  /*if (TLS->our_id.peer_type) {
    assert (!tgl_cmp_peer_id (TLS->our_id, id));
    return;
  }*/

  TLS->our_id = id;

  if (TLS->callback.our_id) {
    TLS->callback.our_id (TLS, TLS->our_id);
  }
}
/* }}} */

void bl_do_set_dh_params (struct tgl_state *TLS, int root, unsigned char prime[], int version) /* {{{ */ {
  if (TLS->encr_prime) { tfree (TLS->encr_prime, 256); TGLC_bn_free (TLS->encr_prime_bn); }

  TLS->encr_root = root;

  TLS->encr_prime = talloc (256);
  memcpy (TLS->encr_prime, prime, 256);
  TLS->encr_prime_bn = TGLC_bn_new ();
  TGLC_bn_bin2bn ((void *)TLS->encr_prime, 256, TLS->encr_prime_bn);
  
  TLS->encr_param_version = version;
    
  assert (tglmp_check_DH_params (TLS, TLS->encr_prime_bn, TLS->encr_root) >= 0);
}
/* }}} */

void bl_do_set_pts (struct tgl_state *TLS, int pts) /* {{{ */ {
  if (TLS->locks & TGL_LOCK_DIFF) { return; }
  if (pts <= TLS->pts) { return; }
  
  TLS->pts = pts;
}
/* }}} */

void bl_do_set_channel_pts (struct tgl_state *TLS, int id, int pts) /* {{{ */ {
  tgl_peer_t *E = tgl_peer_get (TLS, TGL_MK_CHANNEL (id));
  if (!E || !(E->flags & TGLPF_CREATED)) { return; }
  if (E->flags & TGLCHF_DIFF) { return ; }
  if (E->channel.pts <= pts) { return; }
  E->channel.pts = pts;
}
/* }}} */

void bl_do_set_qts (struct tgl_state *TLS, int qts) /* {{{ */ {
  if (TLS->locks & TGL_LOCK_DIFF) { return; }
  if (qts <= TLS->qts) { return; }
  
  TLS->qts = qts;
}
/* }}} */

void bl_do_set_date (struct tgl_state *TLS, int date) /* {{{ */ {
  if (TLS->locks & TGL_LOCK_DIFF) { return; }
  if (date <= TLS->date) { return; }
  
  TLS->date = date;
}
/* }}} */

void bl_do_set_seq (struct tgl_state *TLS, int seq) /* {{{ */ {
  if (TLS->locks & TGL_LOCK_DIFF) { return; }
  if (seq <= TLS->seq) { return; }
  
  TLS->seq = seq;
}
/* }}} */

void bl_do_set_msg_id (struct tgl_state *TLS, tgl_message_id_t *old_id, tgl_message_id_t *new_id) /* {{{ */ {
  if (!memcmp (old_id, new_id, sizeof (tgl_message_id_t))) {
    return;
  }

  struct tgl_message *M = tgl_message_get (TLS, old_id);
  assert (M);
  
  
  if (M->flags & TGLMF_PENDING) {
    tglm_message_remove_unsent (TLS, M);
    M->flags &= ~TGLMF_PENDING;
  }
  
  tglm_message_remove_tree (TLS, M);
  tglm_message_del_peer (TLS, M);
  
  M->permanent_id = *new_id;
  if (tgl_message_get (TLS, new_id)) {
    tglm_message_del_use (TLS, M);
    tglm_message_del_temp_id (TLS, M);
    tglm_message_del_random_id (TLS, M);
    tgls_free_message (TLS, M);
  } else {
    tglm_message_insert_tree (TLS, M);
    tglm_message_add_peer (TLS, M);
  }

  M->server_id = new_id->id;
}
/* }}} */

void bl_do_chat_add_user (struct tgl_state *TLS, tgl_peer_id_t id, int version, int user, int inviter, int date) /* {{{ */ {
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P || !(P->flags & TGLPF_CREATED)) { return; }

  struct tgl_chat *C = &P->chat;
  if (C->user_list_version >= version || !C->user_list_version) { return; }

  int i;
  for (i = 0; i < C->user_list_size; i++) {
    if (C->user_list[i].user_id == user) { 
      return;
    }
  }

  C->user_list_size ++;
  C->user_list = trealloc (C->user_list, 12 * C->user_list_size - 12, 12 * C->user_list_size);
  C->user_list[C->user_list_size - 1].user_id = user;
  C->user_list[C->user_list_size - 1].inviter_id = inviter;
  C->user_list[C->user_list_size - 1].date = date;
  C->user_list_version = version;
  
  if (TLS->callback.chat_update) {
    TLS->callback.chat_update (TLS, C, TGL_UPDATE_MEMBERS);
  }
}
/* }}} */

void bl_do_chat_del_user (struct tgl_state *TLS, tgl_peer_id_t id, int version, int user) /* {{{ */ {
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P || !(P->flags & TGLPF_CREATED)) { return; }

  struct tgl_chat *C = &P->chat;
  if (C->user_list_version >= version || !C->user_list_version) { return; }
  
  int i;
  for (i = 0; i < C->user_list_size; i++) {
    if (C->user_list[i].user_id == user) {
      struct tgl_chat_user t;
      t = C->user_list[i];
      C->user_list[i] = C->user_list[C->user_list_size - 1];
      C->user_list[C->user_list_size - 1] = t;
    }
  }
  if (C->user_list[C->user_list_size - 1].user_id != user) { return; }

  assert (C->user_list[C->user_list_size - 1].user_id == user);
  C->user_list_size --;
  C->user_list = trealloc (C->user_list, 12 * C->user_list_size + 12, 12 * C->user_list_size);
  C->user_list_version = version;
  
  if (TLS->callback.chat_update) {
    TLS->callback.chat_update (TLS, C, TGL_UPDATE_MEMBERS);
  }
}
/* }}} */

void bl_do_edit_message (struct tgl_state *TLS, tgl_message_id_t *id, tgl_peer_id_t *from_id, tgl_peer_id_t *to_id, tgl_peer_id_t *fwd_from_id, int *fwd_date, int *date, const char *message, int message_len, struct tl_ds_message_media *media, struct tl_ds_message_action *action, int *reply_id, struct tl_ds_reply_markup *reply_markup, struct tl_ds_vector *entities, int flags) /* {{{ */ {
  assert (!(flags & 0xfffe0000));
  
  struct tgl_message *M = tgl_message_get (TLS, id);
  
  assert (flags & TGLMF_CREATED);
  assert (!(flags & TGLMF_ENCRYPTED));

  if (flags & (1 << 16)) {
    if (!M) {
      M = tglm_message_alloc (TLS, id);
    }
    M->server_id = id->id;
    assert (!(M->flags & TGLMF_CREATED));
  } else {
    assert (M->flags & TGLMF_CREATED);
  }

  assert (M);
  assert (!(M->flags & TGLMF_ENCRYPTED));

  if ((M->flags & TGLMF_PENDING) && !(flags & TGLMF_PENDING)){
    tglm_message_remove_unsent (TLS, M);
  }
  if (!(M->flags & TGLMF_PENDING) && (flags & TGLMF_PENDING)){
    tglm_message_insert_unsent (TLS, M);
  }

  if ((M->flags & TGLMF_UNREAD) && !(flags & TGLMF_UNREAD)) {
    M->flags = (flags & 0xffff) | TGLMF_UNREAD;
  } else {
    M->flags = (flags & 0xffff);
  }
 
  if (from_id) {
    M->from_id = *from_id;
  } else {
    if (!M->from_id.peer_type) {
      assert (to_id);
      M->from_id = *to_id;
    }
  }

  if (to_id) {
    assert (flags & 0x10000);
    M->to_id = *to_id;
  }

  if (date) {
    M->date = *date;
  }
  
  if (fwd_from_id) {
    assert (fwd_date);
    M->fwd_from_id = *fwd_from_id;
    M->fwd_date = *fwd_date;;
  }
  
  if (action) {
    tglf_fetch_message_action (TLS, &M->action, action);
    M->flags |= TGLMF_SERVICE;
  } 

  if (message) {
    M->message_len = message_len;
    M->message = tstrndup (message, message_len);
    assert (!(M->flags & TGLMF_SERVICE));
  }

  if (media) {
    tglf_fetch_message_media (TLS, &M->media, media);
    assert (!(M->flags & TGLMF_SERVICE));
  }

  if (entities) {
    tglf_fetch_message_entities (TLS, M, entities);
  }

  if (reply_id) {
    M->reply_id = *reply_id;
  }

  if (flags & 0x10000) {
    tglm_message_insert (TLS, M);
  }

  if (!(flags & TGLMF_UNREAD) && (M->flags & TGLMF_UNREAD)) {
    tgls_messages_mark_read (TLS, M, M->flags & TGLMF_OUT, M->permanent_id.id);
  }

  if (reply_markup) {
    M->reply_markup = tglf_fetch_alloc_reply_markup (TLS, M->next, reply_markup);
  }

  if (M->flags & TGLMF_PENDING) {
    tgls_message_change_random_id (TLS, M, M->permanent_id.id);
  }
  
  if (!M->temp_id) {
    tgls_message_change_temp_id (TLS, M, ++TLS->last_temp_id);
  }
}
/* }}} */

void bl_do_edit_message_encr (struct tgl_state *TLS, tgl_message_id_t *id, tgl_peer_id_t *from_id, tgl_peer_id_t *to_id, int *date, const char *message, int message_len, struct tl_ds_decrypted_message_media *media, struct tl_ds_decrypted_message_action *action, struct tl_ds_encrypted_file *file, int flags) /* {{{ */ {
  clear_packet ();
  assert (!(flags & 0xfffe0000));
  
  struct tgl_message *M = tgl_message_get (TLS, id);

  if (flags & (1 << 16)) {
    if (!M) {
      M = tglm_message_alloc (TLS, id);
    } else {
      assert (!(M->flags & TGLMF_CREATED));
    }
    assert (!(M->flags & TGLMF_CREATED));
  } else {
    assert (M->flags & TGLMF_CREATED);
  }

  assert (flags & TGLMF_CREATED);
  assert (flags & TGLMF_ENCRYPTED);

  if ((M->flags & TGLMF_PENDING) && !(flags & TGLMF_PENDING)){
    tglm_message_remove_unsent (TLS, M);
  }
  if (!(M->flags & TGLMF_PENDING) && (flags & TGLMF_PENDING)){
    tglm_message_insert_unsent (TLS, M);
  }

  M->flags = flags & 0xffff;
 
  if (from_id) {
    M->from_id = *from_id;
  }
  if (to_id) {  
    assert (flags & 0x10000);
    M->to_id = *to_id;
  }

  if (date) {
    M->date = *date;
  }

  struct tgl_secret_chat *E = (void *)tgl_peer_get (TLS, M->to_id);
  assert (E);

  if (action) {
    tglf_fetch_message_action_encrypted (TLS, &M->action, action);
    M->flags |= TGLMF_SERVICE;
  }

  if (message) {
    M->message_len = message_len;
    M->message = tstrndup (message, message_len);
    assert (!(M->flags & TGLMF_SERVICE));
  }

  if (media) {
    tglf_fetch_message_media_encrypted (TLS, &M->media, media);
    assert (!(M->flags & TGLMF_SERVICE));
  }

  if (file) {
    tglf_fetch_encrypted_message_file (TLS, &M->media, file);
    assert (!(M->flags & TGLMF_SERVICE));
  }

  if (action && !(M->flags & TGLMF_OUT) && M->action.type == tgl_message_action_notify_layer) {
    E->layer = M->action.layer;
  }

  if ((flags & TGLMF_CREATE) && (flags & TGLMF_OUT)) {
    E->out_seq_no ++;
  }

  if (flags & 0x10000) {
    tglm_message_insert (TLS, M);
  }
}
/* }}} */

void bl_do_message_delete (struct tgl_state *TLS, tgl_message_id_t *id) /* {{{ */ {
  struct tgl_message *M = tgl_message_get (TLS, id);
  if (!M) { return; }
  assert (M);
  if (M->flags & TGLMF_PENDING) {
    tglm_message_remove_unsent (TLS, M);
    M->flags &= ~TGLMF_PENDING;
  }

  tglm_message_remove_tree (TLS, M);
  tglm_message_del_peer (TLS, M);
  tglm_message_del_use (TLS, M);
  tglm_message_del_temp_id (TLS, M);
  tglm_message_del_random_id (TLS, M);
  tgls_free_message (TLS, M);
}
/* }}} */

void bl_do_msg_update (struct tgl_state *TLS, tgl_message_id_t *id) /* {{{ */ {
  struct tgl_message *M = tgl_message_get (TLS, id);
  if (!M) { return; }
  assert (M);
  
  if (!(M->flags & TGLMF_ENCRYPTED)) {
    if (TLS->max_msg_id < M->server_id) {
      TLS->max_msg_id = M->server_id;
    }
  }

  if (TLS->callback.msg_receive) {
    TLS->callback.msg_receive (TLS, M);
  }
}
/* }}} */

void bl_do_reset_authorization (struct tgl_state *TLS) /* {{{ */ {
  int i;
  for (i = 0; i <= TLS->max_dc_num; i++) if (TLS->DC_list[i]) {
    struct tgl_dc *D = TLS->DC_list[i];
    D->flags = 0;
    D->state = st_init;
    D->auth_key_id = D->temp_auth_key_id = 0;
  }
  TLS->seq = 0;
  TLS->qts = 0;
}
/* }}} */

void bl_do_encr_chat_exchange (struct tgl_state *TLS, tgl_peer_id_t id, long long *exchange_id, const void *key, int *state) /* {{{ */ {
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P) { return; }
  
  if (state) {
    P->encr_chat.exchange_state = *state;
  }
  if (exchange_id) {
    P->encr_chat.exchange_id = *exchange_id;
  }

  static unsigned char sha_buffer[20];
  switch (P->encr_chat.exchange_state) {
  case tgl_sce_requested:
    memcpy (P->encr_chat.exchange_key, key, 256);
    break;
  case tgl_sce_accepted:
    memcpy (P->encr_chat.exchange_key, key, 256);
  
    TGLC_sha1 ((unsigned char *)P->encr_chat.exchange_key, 256, sha_buffer);
    P->encr_chat.exchange_key_fingerprint = *(long long *)(sha_buffer + 12);
    break;
  case tgl_sce_committed:
    memcpy (P->encr_chat.exchange_key, P->encr_chat.key, 256);
    P->encr_chat.exchange_key_fingerprint = P->encr_chat.key_fingerprint;

    memcpy (P->encr_chat.key, key, 256);
  
    TGLC_sha1 ((unsigned char *)P->encr_chat.key, 256, sha_buffer);
    P->encr_chat.key_fingerprint = *(long long *)(sha_buffer + 12);
    break;
  case tgl_sce_confirmed:
    P->encr_chat.exchange_state = tgl_sce_none;
    if (P->encr_chat.exchange_state != tgl_sce_committed) {
      memcpy (P->encr_chat.key, P->encr_chat.exchange_key, 256);
      P->encr_chat.key_fingerprint = P->encr_chat.exchange_key_fingerprint;
    }
    break;
  case tgl_sce_aborted:
    P->encr_chat.exchange_state = tgl_sce_none;
    if (P->encr_chat.exchange_state == tgl_sce_committed) {
      memcpy (P->encr_chat.key, P->encr_chat.exchange_key, 256);
      P->encr_chat.key_fingerprint = P->encr_chat.exchange_key_fingerprint;
    }
    break;
  default:
    assert (0);
  }
}
/* }}} */

void bl_do_user (struct tgl_state *TLS, int id, long long *access_hash, const char *first_name, int first_name_len, const char *last_name, int last_name_len, const char *phone, int phone_len, const char *username, int username_len, struct tl_ds_photo *photo, struct tl_ds_user_profile_photo *profile_photo, int *last_read_in, int *last_read_out, struct tl_ds_bot_info *bot_info, int flags) /* {{{ */ {
  tgl_peer_t *_U = tgl_peer_get (TLS, TGL_MK_USER (id));

  unsigned updates = 0;

  if ((flags & TGLPF_CREATE) && (flags != TGL_FLAGS_UNCHANGED)) {
    if (!_U) {
      _U = talloc0 (sizeof (*_U));
      _U->id = TGL_MK_USER (id);
      tglp_insert_user (TLS, _U);
    } else {
      assert (!(_U->flags & TGLPF_CREATED));
    }
    updates |= TGL_UPDATE_CREATED;
  } else {
    assert (_U->flags & TGLPF_CREATED);
  }

  struct tgl_user *U = (void *)_U;
  if (flags == TGL_FLAGS_UNCHANGED) { flags = U->flags; }
  flags &= TGLUF_TYPE_MASK;
  
  if ((flags & TGLUF_TYPE_MASK) != (U->flags & TGLUF_TYPE_MASK)) {
    updates |= TGL_UPDATE_FLAGS;
  }
  U->flags = (U->flags & ~TGLUF_TYPE_MASK) | flags;

  if (access_hash && *access_hash != U->access_hash) {
    U->access_hash = *access_hash;
    U->id.access_hash = *access_hash;
    updates |= TGL_UPDATE_ACCESS_HASH;
  }

  if (first_name || last_name) {
    if (!U->first_name || !U->last_name || mystreq1 (U->first_name, first_name, first_name_len) || mystreq1 (U->last_name, last_name, last_name_len)) {
      if (U->first_name) {
        tfree_str (U->first_name);
      }
      U->first_name = tstrndup (first_name, first_name_len);
      if (U->last_name) {
        tfree_str (U->last_name);
      }
      U->last_name = tstrndup (last_name, last_name_len);
    
      updates |= TGL_UPDATE_NAME;

      if (U->print_name) { 
        tglp_peer_delete_name (TLS, (void *)U);
        tfree_str (U->print_name); 
      }
      U->print_name = TLS->callback.create_print_name (TLS, U->id, U->first_name, U->last_name, 0, 0);
      tglp_peer_insert_name (TLS, (void *)U);
    }
  }

  if (phone && (!U->phone || mystreq1 (U->phone, phone, phone_len))) {
    if (U->phone) {
      tfree_str (U->phone);
    }
    U->phone = tstrndup (phone, phone_len);
    updates |= TGL_UPDATE_PHONE;
  }

  if (username && (!U->username || mystreq1 (U->username, username, username_len))) {
    if (U->username) {
      tfree_str (U->username);
    }
    U->username = tstrndup (username, username_len);
    updates |= TGL_UPDATE_USERNAME;
  }

  if (photo) {
    if (!U->photo || U->photo->id != DS_LVAL (photo->id)) {
      if (U->photo) {
        tgls_free_photo (TLS, U->photo);
      }
      U->photo = tglf_fetch_alloc_photo (TLS, photo);
      U->flags |= TGLUF_HAS_PHOTO;
    }
  }
  
  if (profile_photo) {
    if (U->photo_id != DS_LVAL (profile_photo->photo_id)) {
      U->photo_id = DS_LVAL (profile_photo->photo_id);
      tglf_fetch_file_location (TLS, &U->photo_big, profile_photo->photo_big);
      tglf_fetch_file_location (TLS, &U->photo_small, profile_photo->photo_small);
      updates |= TGL_UPDATE_PHOTO;
    }
  }

  if (last_read_in) {
    U->last_read_in = *last_read_in;
    tgls_messages_mark_read (TLS, U->last, 0, U->last_read_in);
  }
  if (last_read_out) {
    U->last_read_out = *last_read_out;
    tgls_messages_mark_read (TLS, U->last, TGLMF_OUT, U->last_read_out);
  }
  
  if (bot_info) {
    if (!U->bot_info || U->bot_info->version != DS_LVAL (bot_info->version)) {
      if (U->bot_info) {
        tgls_free_bot_info (TLS, U->bot_info);
      }
      U->bot_info = tglf_fetch_alloc_bot_info (TLS, bot_info);
    }
  }

  if (TLS->callback.user_update && updates) {
    TLS->callback.user_update (TLS, U, updates);
  }
}
/* }}} */

void bl_do_chat (struct tgl_state *TLS, int id, const char *title, int title_len, int *user_num, int *date, int *version, struct tl_ds_vector *participants, struct tl_ds_chat_photo *chat_photo, struct tl_ds_photo *photo, int *admin, int *last_read_in, int *last_read_out, int flags) /* {{{ */ {
  tgl_peer_t *_U = tgl_peer_get (TLS, TGL_MK_CHAT (id));

  unsigned updates = 0;

  if ((flags & TGLPF_CREATE) && (flags != TGL_FLAGS_UNCHANGED)) {
    if (!_U) {
      _U = talloc0 (sizeof (*_U));
      _U->id = TGL_MK_CHAT (id);
      tglp_insert_chat (TLS, _U);
    } else {
      assert (!(_U->flags & TGLPF_CREATED));
    }
    updates |= TGL_UPDATE_CREATED;
  } else {
    assert (_U->flags & TGLPF_CREATED);
  }

  struct tgl_chat *C = &_U->chat;
  if (flags == TGL_FLAGS_UNCHANGED) { flags = C->flags; }
  flags &= TGLCF_TYPE_MASK;
  
  if ((flags & TGLCF_TYPE_MASK) != (C->flags & TGLCF_TYPE_MASK)) {
    updates |= TGL_UPDATE_FLAGS;
  }
  C->flags = (C->flags & ~TGLCF_TYPE_MASK) | flags;

  if (title && (!C->title || mystreq1 (C->title, title, title_len))) {
    if (C->title) {
      tfree_str (C->title);
    }
    C->title = tstrndup (title, title_len);

    if (C->print_title) { 
      tglp_peer_delete_name (TLS, (void *)C);
      tfree_str (C->print_title); 
    }
    C->print_title = TLS->callback.create_print_name (TLS, C->id, C->title, 0, 0, 0);
    tglp_peer_insert_name (TLS, (void *)C);
    
    updates |= TGL_UPDATE_TITLE;
  }
  
  if (user_num) {
    C->users_num = *user_num;
  }
  
  if (date) {
    C->date = *date;
  }

  if (chat_photo && chat_photo->photo_big) {
    if (DS_LVAL (chat_photo->photo_big->secret) != C->photo_big.secret) {
      tglf_fetch_file_location (TLS, &C->photo_big, chat_photo->photo_big);
      tglf_fetch_file_location (TLS, &C->photo_small, chat_photo->photo_small);
      updates |= TGL_UPDATE_PHOTO;
    }
  }

  if (photo) {
    if (!C->photo || C->photo->id != DS_LVAL (photo->id)) {
      if (C->photo) {
        tgls_free_photo (TLS, C->photo);
      }
      C->photo = tglf_fetch_alloc_photo (TLS, photo);
      C->flags |= TGLPF_HAS_PHOTO;
      //updates |= TGL_UPDATE_PHOTO;
    }
  }

  if (admin && *admin != C->admin_id) {
    C->admin_id = *admin;
    updates |= TGL_UPDATE_ADMIN;
  }
  
  if (version) {
    assert (participants);
    if (*version > C->version) {
      C->version = *version;

      if (C->user_list) { tfree (C->user_list, 12 * C->user_list_size); }

      C->user_list_size = DS_LVAL (participants->f1);
      C->user_list = talloc (12 * C->user_list_size);

      int i;
      for (i = 0; i < C->user_list_size; i++) {
        struct tl_ds_chat_participant *DS_P = participants->f2[i];
        C->user_list[i].user_id = DS_LVAL (DS_P->user_id);
        C->user_list[i].inviter_id = DS_LVAL (DS_P->inviter_id);
        C->user_list[i].date = DS_LVAL (DS_P->date);
      }

      updates |= TGL_UPDATE_MEMBERS;
    }
  }
 
  if (last_read_in) {
    C->last_read_in = *last_read_in;
    tgls_messages_mark_read (TLS, C->last, 0, C->last_read_in);
  }
 
  if (last_read_out) {
    C->last_read_out = *last_read_out;
    tgls_messages_mark_read (TLS, C->last, TGLMF_OUT, C->last_read_out);
  }

      
  if (TLS->callback.chat_update && updates) {
    TLS->callback.chat_update (TLS, C, updates);
  }
}
/* }}} */

void bl_do_encr_chat (struct tgl_state *TLS, int id, long long *access_hash, int *date, int *admin, int *user_id, void *key, void *g_key, void *first_key_id, int *state, int *ttl, int *layer, int *in_seq_no, int *last_in_seq_no, int *out_seq_no, long long *key_fingerprint, int flags, const char *print_name, int print_name_len) /* {{{ */ {
  tgl_peer_t *_U = tgl_peer_get (TLS, TGL_MK_ENCR_CHAT (id));

  unsigned updates = 0;

  if ((flags & TGLPF_CREATE) && (flags != TGL_FLAGS_UNCHANGED)) {
    if (!_U) {
      _U = talloc0 (sizeof (*_U));
      _U->id = TGL_MK_ENCR_CHAT (id);
      tglp_insert_encrypted_chat (TLS, _U);
    } else {
      assert (!(_U->flags & TGLPF_CREATED));
    }
    updates |= TGL_UPDATE_CREATED;
  } else {
    assert (_U->flags & TGLPF_CREATED);
  }

  struct tgl_secret_chat *U = (void *)_U;
  if (flags == TGL_FLAGS_UNCHANGED) { flags = U->flags; }
  flags &= TGLECF_TYPE_MASK;
  
  if ((flags & TGLECF_TYPE_MASK) != (U->flags & TGLECF_TYPE_MASK)) {
    updates |= TGL_UPDATE_FLAGS;
  }
  U->flags = (U->flags & ~TGLECF_TYPE_MASK) | flags;

  if (access_hash && *access_hash != U->access_hash) {
    U->access_hash = *access_hash;
    U->id.access_hash = *access_hash;
    updates |= TGL_UPDATE_ACCESS_HASH;
  }

  if (date) {
    U->date = *date;
  }

  if (admin) {
    U->admin_id = *admin;
  }

  if (user_id) {
    U->user_id = *user_id;
  }

  if (key_fingerprint) {
    U->key_fingerprint = *key_fingerprint;
  }

  if (in_seq_no) {
    U->in_seq_no = *in_seq_no;
  }
  if (out_seq_no) {
    U->out_seq_no = *out_seq_no;
  }
  if (last_in_seq_no) {
    U->last_in_seq_no = *last_in_seq_no;
  }

  tgl_peer_t *Us = tgl_peer_get (TLS, TGL_MK_USER (U->user_id));
  
  if (!U->print_name) {
    if (print_name) {
      U->print_name = tstrndup (print_name, print_name_len);
    } else {
      if (Us) {
        U->print_name = TLS->callback.create_print_name (TLS, TGL_MK_ENCR_CHAT (id), "!", Us->user.first_name, Us->user.last_name, 0);
      } else {
        static char buf[100];
        tsnprintf (buf, 99, "user#%d", U->user_id);
        U->print_name = TLS->callback.create_print_name (TLS, TGL_MK_ENCR_CHAT (id), "!", buf, 0, 0);
      }
      tglp_peer_insert_name (TLS, (void *)U);
    }
  }

  if (g_key) {
    if (!U->g_key)  {
      U->g_key = talloc (256);
    }
    memcpy (U->g_key, g_key, 256);
  }

  if (key) {
    memcpy (U->key, key, 256);
  }

  if (first_key_id) {
    memcpy (U->first_key_sha, first_key_id, 20);
  }

  if (state) {
    if (U->state == sc_waiting && *state == sc_ok) {
      tgl_do_create_keys_end (TLS, U);
    }
    if ((int)U->state != *state) {
      switch (*state) {
      case sc_request:
        updates |= TGL_UPDATE_REQUESTED;
        break;
      case sc_ok:
        updates |= TGL_UPDATE_WORKING;
        vlogprintf (E_WARNING, "Secret chat in ok state\n");
        break;
      default:
        break;
      } 
    }
    U->state = *state;
  }
  
  if (TLS->callback.secret_chat_update && updates) {
    TLS->callback.secret_chat_update (TLS, U, updates);
  }
}
/* }}} */

void bl_do_channel (struct tgl_state *TLS, int id, long long *access_hash, int *date, const char *title, int title_len, const char *username, int username_len, struct tl_ds_chat_photo *chat_photo, struct tl_ds_photo *photo, int *version, char *about, int about_len, int *participants_count, int *admins_count, int *kicked_count, int *last_read_in, int flags) /* {{{ */ {
  tgl_peer_t *_U = tgl_peer_get (TLS, TGL_MK_CHANNEL (id));

  unsigned updates = 0;

  if ((flags & TGLPF_CREATE) && (flags != TGL_FLAGS_UNCHANGED)) {
    if (!_U) {
      _U = talloc0 (sizeof (*_U));
      _U->id = TGL_MK_CHANNEL (id);
      tglp_insert_channel (TLS, _U);
    } else {
      assert (!(_U->flags & TGLPF_CREATED));
    }
    updates |= TGL_UPDATE_CREATED;
  } else {
    assert (_U->flags & TGLPF_CREATED);
  }
  
  struct tgl_channel *C = &_U->channel;
  if (flags == TGL_FLAGS_UNCHANGED) { flags = C->flags; }
  flags &= TGLCHF_TYPE_MASK;
  
  if ((flags & TGLCHF_TYPE_MASK) != (C->flags & TGLCHF_TYPE_MASK)) {
    updates |= TGL_UPDATE_FLAGS;
  }
  C->flags = (C->flags & ~TGLCHF_TYPE_MASK) | flags;

  if (access_hash && *access_hash != C->access_hash) {
    C->access_hash = *access_hash;
    C->id.access_hash = *access_hash;
    updates |= TGL_UPDATE_ACCESS_HASH;
  }

  if (date) {
    C->date = *date;
  }

  if (title && (!C->title || mystreq1 (C->title, title, title_len))) {
    if (C->title) {
      tfree_str (C->title);
    }
    C->title = tstrndup (title, title_len);
    
    if (C->print_title) { 
      tglp_peer_delete_name (TLS, (void *)C);
      tfree_str (C->print_title); 
    }
    C->print_title = TLS->callback.create_print_name (TLS, C->id, C->title, 0, 0, 0);
    tglp_peer_insert_name (TLS, (void *)C);
    
    updates |= TGL_UPDATE_TITLE;
  }
  
  if (chat_photo) {
    if (chat_photo->photo_big && DS_LVAL (chat_photo->photo_big->secret) != C->photo_big.secret) {
      tglf_fetch_file_location (TLS, &C->photo_big, chat_photo->photo_big);
      tglf_fetch_file_location (TLS, &C->photo_small, chat_photo->photo_small);
      updates |= TGL_UPDATE_PHOTO;
    }
  }

  if (photo) {
    if (!C->photo || C->photo->id != DS_LVAL (photo->id)) {
      if (C->photo) {
        tgls_free_photo (TLS, C->photo);
      }
      C->photo = tglf_fetch_alloc_photo (TLS, photo);
      C->flags |= TGLPF_HAS_PHOTO;
    }
  }

  if (username) {
    if (!C->username || mystreq1 (C->username, username, username_len)) {
      if (C->username) {
        tfree_str (C->username);
      }
      C->username = tstrndup (username, username_len);
      updates |= TGL_UPDATE_USERNAME;
    }
  }
  
  if (about) {
    if (!C->about || mystreq1 (C->about, about, about_len)) {
      tfree_str (C->about);
    }
    C->about = tstrndup (about, about_len);
  }

  if (admins_count) {
    C->admins_count = *admins_count;
  }
  
  if (participants_count) {
    C->participants_count = *participants_count;
  }
  
  if (kicked_count) {
    C->kicked_count = *kicked_count;
  }
  
  if (last_read_in) {
    C->last_read_in = *last_read_in;
    tgls_messages_mark_read (TLS, C->last, 0, C->last_read_in);
  }
  
  if (TLS->callback.channel_update && updates) {
    TLS->callback.channel_update (TLS, C, updates);
  }
}
/* }}} */

void bl_do_peer_delete (struct tgl_state *TLS, tgl_peer_id_t id) /* {{{ */ {
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P || !(P->flags & TGLPF_CREATED)) { return; }

  if (P->flags & TGLPF_DELETED) { return; }
  P->flags |= TGLPF_DELETED;

  switch (id.peer_type) {
  case TGL_PEER_USER:
    if (TLS->callback.user_update) {
      TLS->callback.user_update (TLS, (void *)P, TGL_UPDATE_DELETED);
    }
    break;
  case TGL_PEER_CHAT:
    if (TLS->callback.chat_update) {
      TLS->callback.chat_update (TLS, (void *)P, TGL_UPDATE_DELETED);
    }
    break;
  case TGL_PEER_ENCR_CHAT:
    if (TLS->callback.secret_chat_update) {
      TLS->callback.secret_chat_update (TLS, (void *)P, TGL_UPDATE_DELETED);
    }
    break;
  case TGL_PEER_CHANNEL:
    if (TLS->callback.channel_update) {
      TLS->callback.channel_update (TLS, (void *)P, TGL_UPDATE_DELETED);
    }
    break;
  default:
    assert (0);
  }
}
/* }}} */
