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
#include "tgl.h"
#include "updates.h"
#include "mtproto-common.h"
#include "tgl-binlog.h"
#include "auto.h"
#include "auto/auto-types.h"
#include "auto/auto-skip.h"
#include "auto/auto-fetch-ds.h"
#include "auto/auto-free-ds.h"
#include "tgl-structures.h"
#include "tgl-methods-in.h"
#include "tree.h"

#include <assert.h>

void tgl_do_get_channel_difference (struct tgl_state *TLS, int channel_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

static void fetch_dc_option (struct tgl_state *TLS, struct tl_ds_dc_option *DS_DO) {
  vlogprintf (E_DEBUG, "id = %d, %.*s:%d\n", DS_LVAL (DS_DO->id), DS_RSTR (DS_DO->ip_address), DS_LVAL (DS_DO->port));

  bl_do_dc_option (TLS, DS_LVAL (DS_DO->flags), DS_LVAL (DS_DO->id), NULL, 0, DS_STR (DS_DO->ip_address), DS_LVAL (DS_DO->port));
}

int tgl_check_pts_diff (struct tgl_state *TLS, int pts, int pts_count) {
  vlogprintf (E_DEBUG - 1, "pts = %d, pts_count = %d\n", pts, pts_count);
  if (!TLS->pts) {
    return 1;
  }
  //assert (TLS->pts);
  if (pts < TLS->pts + pts_count) {
    vlogprintf (E_NOTICE, "Duplicate message with pts=%d\n", pts);
    return -1;
  }
  if (pts > TLS->pts + pts_count) {
    vlogprintf (E_NOTICE, "Hole in pts (pts = %d, count = %d, cur_pts = %d)\n", pts, pts_count, TLS->pts);
    tgl_do_get_difference (TLS, 0, 0, 0);
    return -1;
  }
  if (TLS->locks & TGL_LOCK_DIFF) {
    vlogprintf (E_DEBUG, "Update during get_difference. pts = %d\n", pts);
    return -1;
  }
  vlogprintf (E_DEBUG, "Ok update. pts = %d\n", pts);
  return 1;
}

int tgl_check_qts_diff (struct tgl_state *TLS, int qts, int qts_count) {
  vlogprintf (E_ERROR, "qts = %d, qts_count = %d\n", qts, qts_count);
  if (qts < TLS->qts + qts_count) {
    vlogprintf (E_NOTICE, "Duplicate message with qts=%d\n", qts);
    return -1;
  }
  if (qts > TLS->qts + qts_count) {
    vlogprintf (E_NOTICE, "Hole in qts (qts = %d, count = %d, cur_qts = %d)\n", qts, qts_count, TLS->qts);
    tgl_do_get_difference (TLS, 0, 0, 0);
    return -1;
  }
  if (TLS->locks & TGL_LOCK_DIFF) {
    vlogprintf (E_DEBUG, "Update during get_difference. qts = %d\n", qts);
    return -1;
  }
  vlogprintf (E_DEBUG, "Ok update. qts = %d\n", qts);
  return 1;
}

int tgl_check_channel_pts_diff (struct tgl_state *TLS, tgl_peer_t *_E, int pts, int pts_count) {
  struct tgl_channel *E = &_E->channel;
  vlogprintf (E_DEBUG - 1, "channel %d: pts = %d, pts_count = %d, current_pts = %d\n", tgl_get_peer_id (E->id), pts, pts_count, E->pts);
  if (!E->pts) {
    return 1;
  }
  //assert (TLS->pts);
  if (pts < E->pts + pts_count) {
    vlogprintf (E_NOTICE, "Duplicate message with pts=%d\n", pts);
    return -1;
  }
  if (pts > E->pts + pts_count) {
    vlogprintf (E_NOTICE, "Hole in pts (pts = %d, count = %d, cur_pts = %d)\n", pts, pts_count, E->pts);
    tgl_do_get_channel_difference (TLS, tgl_get_peer_id (E->id), 0, 0);
    return -1;
  }
  if (E->flags & TGLCHF_DIFF) {
    vlogprintf (E_DEBUG, "Update during get_difference. pts = %d\n", pts);
    return -1;
  }
  vlogprintf (E_DEBUG, "Ok update. pts = %d\n", pts);
  return 1;
}
  
static int do_skip_seq (struct tgl_state *TLS, int seq) {
  if (!seq) {
    vlogprintf (E_DEBUG, "Ok update. seq = %d\n", seq);
    return 0;
  }
  if (TLS->seq) {
    if (seq <= TLS->seq) {
      vlogprintf (E_NOTICE, "Duplicate message with seq=%d\n", seq);
      return -1;
    }
    if (seq > TLS->seq + 1) {
      vlogprintf (E_NOTICE, "Hole in seq (seq = %d, cur_seq = %d)\n", seq, TLS->seq);
      //vlogprintf (E_NOTICE, "lock_diff = %s\n", (TLS->locks & TGL_LOCK_DIFF) ? "true" : "false");
      tgl_do_get_difference (TLS, 0, 0, 0);
      return -1;
    }
    if (TLS->locks & TGL_LOCK_DIFF) {
      vlogprintf (E_DEBUG, "Update during get_difference. seq = %d\n", seq);
      return -1;
    }
    vlogprintf (E_DEBUG, "Ok update. seq = %d\n", seq);
    return 0;
  } else {
    return -1;
  }
}

void tglu_work_update (struct tgl_state *TLS, int check_only, struct tl_ds_update *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    vlogprintf (E_DEBUG, "Update during get_difference. DROP\n");
    return;
  }

  if (DS_U->pts) {
    assert (DS_U->pts_count);

    if (!check_only && tgl_check_pts_diff (TLS, DS_LVAL (DS_U->pts), DS_LVAL (DS_U->pts_count)) <= 0) {
      return;
    }
  }
  
  if (DS_U->qts) {
    if (!check_only && tgl_check_qts_diff (TLS, DS_LVAL (DS_U->qts), 1) <= 0) {
      return;
    }
  }

  if (DS_U->channel_pts) {
    assert (DS_U->channel_pts_count);
    int channel_id;
    if (DS_U->channel_id) {
      channel_id = DS_LVAL (DS_U->channel_id);
    } else {
      assert (DS_U->message);
      if (!DS_U->message->to_id) {
        return;
      }
      assert (DS_U->message->to_id);
      assert (DS_U->message->to_id->magic == CODE_peer_channel);
      channel_id = DS_LVAL (DS_U->message->to_id->channel_id);
    }    

    tgl_peer_t *E = tgl_peer_get (TLS, TGL_MK_CHANNEL (channel_id));
    if (!E) {
      return;
    }

    if (!check_only && tgl_check_channel_pts_diff (TLS, E, DS_LVAL (DS_U->channel_pts), DS_LVAL (DS_U->channel_pts_count)) <= 0) {
      return;
    }
  }

  vlogprintf (E_NOTICE, "update 0x%08x (check=%d)\n", DS_U->magic, check_only);
  if (check_only > 0 && DS_U->magic != CODE_update_message_i_d) { return; }
  switch (DS_U->magic) {
  case CODE_update_new_message:
    {
      //struct tgl_message *N = tgl_message_get (TLS, DS_LVAL (DS_U->id));
      //int new = (!N || !(N->flags & TGLMF_CREATED));
      int new_msg = 0;
      struct tgl_message *M = tglf_fetch_alloc_message (TLS, DS_U->message, &new_msg);
      assert (M);
      if (new_msg) {
        bl_do_msg_update (TLS, &M->permanent_id);
      }
      break;
    };
  case CODE_update_message_i_d:
    {
      tgl_message_id_t msg_id;
      msg_id.peer_type = TGL_PEER_RANDOM_ID;
      msg_id.id = DS_LVAL (DS_U->random_id);
      struct tgl_message *M = tgl_message_get (TLS, &msg_id);
      if (M && (M->flags & TGLMF_PENDING)) {
        msg_id = M->permanent_id;
        msg_id.id = DS_LVAL (DS_U->id);
        bl_do_set_msg_id (TLS, &M->permanent_id, &msg_id);
        bl_do_msg_update (TLS, &msg_id);
      }
    }
    break;
/*  case CODE_update_read_messages:
    {
      int n = DS_LVAL (DS_U->messages->cnt);
      
      int i;
      for (i = 0; i < n; i++) {
        struct tgl_message *M = tgl_message_get (TLS, DS_LVAL (DS_U->messages->data[i]));
        if (M) {
          tgl_peer_t *P;
          if (M->flags & TGLMF_OUT) {
            P = tgl_peer_get (TLS, M->to_id);
            if (P && (P->flags & TGLMF_CREATED)) {
              if (tgl_get_peer_type (P->id) == TGL_PEER_USER) {
                bl_do_user (TLS, tgl_get_peer_id (P->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, 0, NULL, 0, NULL, NULL, (int *)&M->id, TGL_FLAGS_UNCHANGED);
              } else {
                bl_do_chat (TLS, tgl_get_peer_id (P->id), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (int *)&M->id, TGL_FLAGS_UNCHANGED);
              }
            }
          } else {
            if (tgl_get_peer_type (M->to_id) == TGL_PEER_USER) {
              P = tgl_peer_get (TLS, M->from_id);
            } else {
              P = tgl_peer_get (TLS, M->to_id);
            }
            if (P && (P->flags & TGLMF_CREATED)) {
              if (tgl_get_peer_type (P->id) == TGL_PEER_USER) {
                bl_do_user (TLS, tgl_get_peer_id (P->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, 0, NULL, 0, NULL, (int *)&M->id, NULL, TGL_FLAGS_UNCHANGED);
              } else {
                bl_do_chat (TLS, tgl_get_peer_id (P->id), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (int *)&M->id, NULL, TGL_FLAGS_UNCHANGED);
              }
            }
          }
        }
      }
    }
    break;*/
  case CODE_update_user_typing:
    {
      tgl_peer_id_t id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *U = tgl_peer_get (TLS, id);
      enum tgl_typing_status status = tglf_fetch_typing (DS_U->action);

      if (TLS->callback.type_notification && U) {
        TLS->callback.type_notification (TLS, (void *)U, status);
      }
    }
    break;
  case CODE_update_chat_user_typing:
    {
      tgl_peer_id_t chat_id = TGL_MK_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_id_t id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *C = tgl_peer_get (TLS, chat_id);
      tgl_peer_t *U = tgl_peer_get (TLS, id);
      enum tgl_typing_status status = tglf_fetch_typing (DS_U->action);      
      
      if (U && C) {
        if (TLS->callback.type_in_chat_notification) {
          TLS->callback.type_in_chat_notification (TLS, (void *)U, (void *)C, status);
        }
      }
    }
    break;
  case CODE_update_user_status:
    {
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *U = tgl_peer_get (TLS, user_id);
      if (U) {
        tglf_fetch_user_status (TLS, &U->user.status, &U->user, DS_U->status);

        if (TLS->callback.status_notification) {
          TLS->callback.status_notification (TLS, (void *)U);
        }
      }
    }
    break;
  case CODE_update_user_name:
    {
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *UC = tgl_peer_get (TLS, user_id);
      if (UC && (UC->flags & TGLPF_CREATED)) {
        bl_do_user (TLS, tgl_get_peer_id (user_id), NULL, DS_STR (DS_U->first_name), DS_STR (DS_U->last_name), NULL, 0, DS_STR (DS_U->username), NULL, NULL, NULL, NULL, NULL, TGL_FLAGS_UNCHANGED);
      }
    }
    break;
  case CODE_update_user_photo:
    {
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *UC = tgl_peer_get (TLS, user_id);
      
      if (UC && (UC->flags & TGLUF_CREATED)) {
        bl_do_user (TLS, tgl_get_peer_id (user_id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, DS_U->photo, NULL, NULL, NULL, TGL_FLAGS_UNCHANGED);
      }
    }
    break;
  case CODE_update_delete_messages:
    {
    }
    break;
  case CODE_update_chat_participants:
    {
      tgl_peer_id_t chat_id = TGL_MK_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_t *C = tgl_peer_get (TLS, chat_id);
      if (C && (C->flags & TGLPF_CREATED) && DS_U->participants->magic == CODE_chat_participants) {
        bl_do_chat (TLS, tgl_get_peer_id (chat_id), NULL, 0, NULL, NULL, DS_U->participants->version, (struct tl_ds_vector *)DS_U->participants->participants, NULL, NULL, NULL, NULL, NULL, TGL_FLAGS_UNCHANGED);
      }
    }
    break;
  case CODE_update_contact_registered:
    {
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *U = tgl_peer_get (TLS, user_id);
      if (TLS->callback.user_registered && U) {
        TLS->callback.user_registered (TLS, (void *)U);
      }
    }
    break;
  case CODE_update_contact_link:
    {
    }
    break;
  /*case CODE_update_activation:
    {
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_t *U = tgl_peer_get (TLS, user_id);
     
      if (TLS->callback.user_activated && U) {
        TLS->callback.user_activated (TLS, (void *)U);
      }
    }
    break;*/
  case CODE_update_new_authorization:
    {
      if (TLS->callback.new_authorization) {
        TLS->callback.new_authorization (TLS, DS_U->device->data, DS_U->location->data);
      }
    }
    break;
  /*case CODE_update_new_geo_chat_message:
    {
    }
    break;*/
  case CODE_update_new_encrypted_message:
    {
      struct tgl_message *M = tglf_fetch_alloc_encrypted_message (TLS, DS_U->encr_message);
      if (M) {
        bl_do_msg_update (TLS, &M->permanent_id);
      }
    }
    break;
  case CODE_update_encryption:
    {
      struct tgl_secret_chat *E = tglf_fetch_alloc_encrypted_chat (TLS, DS_U->encr_chat);     
      vlogprintf (E_DEBUG, "Secret chat state = %d\n", E->state);
      if (E->state == sc_ok) {
        tgl_do_send_encr_chat_layer (TLS, E);
      }
    }
    break;
  case CODE_update_encrypted_chat_typing:
    {
      tgl_peer_id_t id = TGL_MK_ENCR_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_t *P = tgl_peer_get (TLS, id);
      
      if (P) {
        if (TLS->callback.type_in_secret_chat_notification) {
          TLS->callback.type_in_secret_chat_notification (TLS, (void *)P);
        }
      }
    }
    break;
  case CODE_update_encrypted_messages_read:
    {
      tgl_peer_id_t id = TGL_MK_ENCR_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_t *P = tgl_peer_get (TLS, id);
      
      if (P && P->last) {
        struct tgl_message *M = P->last;
        while (M && (!(M->flags & TGLMF_OUT) || (M->flags & TGLMF_UNREAD))) {
          if (M->flags & TGLMF_OUT) {
            bl_do_edit_message_encr (TLS, &M->permanent_id, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, M->flags & ~TGLMF_UNREAD);
          }
          M = M->next;
        }
      }
    }
    break;
  case CODE_update_chat_participant_add:
    {
      tgl_peer_id_t chat_id = TGL_MK_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      tgl_peer_id_t inviter_id = TGL_MK_USER (DS_LVAL (DS_U->inviter_id));
      int version = DS_LVAL (DS_U->version); 
      
      tgl_peer_t *C = tgl_peer_get (TLS, chat_id);
      if (C && (C->flags & TGLPF_CREATED)) {
        bl_do_chat_add_user (TLS, C->id, version, tgl_get_peer_id (user_id), tgl_get_peer_id (inviter_id), time (0));
      }
    }
    break;
  case CODE_update_chat_participant_delete:
    {
      tgl_peer_id_t chat_id = TGL_MK_CHAT (DS_LVAL (DS_U->chat_id));
      tgl_peer_id_t user_id = TGL_MK_USER (DS_LVAL (DS_U->user_id));
      int version = DS_LVAL (DS_U->version); 
      
      tgl_peer_t *C = tgl_peer_get (TLS, chat_id);
      if (C && (C->flags & TGLPF_CREATED)) {
        bl_do_chat_del_user (TLS, C->id, version, tgl_get_peer_id (user_id));
      }
    }
    break;
  case CODE_update_dc_options:
    {
      int i;
      for (i = 0; i < DS_LVAL (DS_U->dc_options->cnt); i++) {
        fetch_dc_option (TLS, DS_U->dc_options->data[i]);
      }
    }
    break;
  case CODE_update_user_blocked:
    {
      int blocked = DS_BVAL (DS_U->blocked);
      tgl_peer_t *P = tgl_peer_get (TLS, TGL_MK_USER (DS_LVAL (DS_U->user_id)));
      if (P && (P->flags & TGLPF_CREATED)) {
        int flags = P->flags & 0xffff; 
        if (blocked) {
          flags |= TGLUF_BLOCKED;
        } else {
          flags &= ~TGLUF_BLOCKED;
        }
        bl_do_user (TLS, tgl_get_peer_id (P->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, flags);
      }
    }
    break;
  case CODE_update_notify_settings:
    {
    }
    break;
  case CODE_update_service_notification:
    {
      vlogprintf (E_ERROR, "Notification %.*s: %.*s\n", DS_RSTR (DS_U->type), DS_RSTR (DS_U->message_text));
      if (TLS->callback.notification) {
        TLS->callback.notification (TLS, DS_U->type->data, DS_U->message_text->data);
      }
    }
    break;
  case CODE_update_privacy:
    vlogprintf (E_NOTICE, "privacy change update\n");
    break;
  case CODE_update_user_phone:
    {
      tgl_peer_t *U = tgl_peer_get (TLS, TGL_MK_USER (DS_LVAL (DS_U->user_id)));
      if (U && (U->flags & TGLPF_CREATED)) {
        bl_do_user (TLS, tgl_get_peer_id (U->id), NULL, NULL, 0, NULL, 0, DS_STR (DS_U->phone), NULL, 0, NULL, NULL, NULL, NULL, NULL, TGL_FLAGS_UNCHANGED);
      }
    }
    break;
  case CODE_update_read_history_inbox:
    {
      tgl_peer_id_t id = tglf_fetch_peer_id (TLS, DS_U->peer);
      tgl_peer_t *P = tgl_peer_get (TLS, id);
      if (P && (P->flags & TGLPF_CREATED)) {
        if (tgl_get_peer_type (P->id) == TGL_PEER_USER) {
          bl_do_user (TLS, tgl_get_peer_id (P->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, DS_U->max_id, NULL, NULL, TGL_FLAGS_UNCHANGED);
        } else {
          bl_do_chat (TLS, tgl_get_peer_id (P->id), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, DS_U->max_id, NULL, TGL_FLAGS_UNCHANGED);
        }
      }
    }
    break;
  case CODE_update_read_history_outbox:
    {
      tgl_peer_id_t id = tglf_fetch_peer_id (TLS, DS_U->peer);
      tgl_peer_t *P = tgl_peer_get (TLS, id);
      if (P && (P->flags & TGLPF_CREATED)) {
        if (tgl_get_peer_type (P->id) == TGL_PEER_USER) {
          bl_do_user (TLS, tgl_get_peer_id (P->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, NULL, DS_U->max_id, NULL, TGL_FLAGS_UNCHANGED);
        } else {
          bl_do_chat (TLS, tgl_get_peer_id (P->id), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, DS_U->max_id, TGL_FLAGS_UNCHANGED);
        }
      }
    }
    break;
  case CODE_update_web_page:
    {
    }
    break;
  /*case CODE_update_msg_update:
    {
      struct tgl_message *M = tgl_message_get (TLS, DS_LVAL (DS_U->id));
      if (M) {
        bl_do_msg_update (TLS, M->id);
      }
    }
    break;*/
  case CODE_update_read_messages_contents:
    break;
  case CODE_update_channel_too_long:
    {
      tgl_do_get_channel_difference (TLS, DS_LVAL (DS_U->channel_id), NULL, NULL);
    }
    break;
  case CODE_update_channel:
    break;
  case CODE_update_channel_group:
    break;
  case CODE_update_new_channel_message:
    {
      int new_msg = 0;
      struct tgl_message *M = tglf_fetch_alloc_message (TLS, DS_U->message, &new_msg);
      if (M && new_msg) {
        bl_do_msg_update (TLS, &M->permanent_id);
      }
    }
    break;
  case CODE_update_read_channel_inbox:
    break;
  case CODE_update_delete_channel_messages:
    break;
  case CODE_update_channel_message_views:
    break;
  case CODE_update_chat_admins:
    break;
  case CODE_update_chat_participant_admin:
    break;
  case CODE_update_new_sticker_set:
    break;
  case CODE_update_sticker_sets_order:
    break;
  case CODE_update_sticker_sets:
    break;
  case CODE_update_saved_gifs:
    break;
  case CODE_update_bot_inline_query:
    break;
  default:
    assert (0);
  }
  
  if (check_only) { return; }

  if (DS_U->pts) {
    assert (DS_U->pts_count);

    bl_do_set_pts (TLS, DS_LVAL (DS_U->pts));
  }
  if (DS_U->qts) {
    bl_do_set_qts (TLS, DS_LVAL (DS_U->qts));
  }
  if (DS_U->channel_pts) {
    assert (DS_U->channel_pts_count);
    
    int channel_id;
    if (DS_U->channel_id) {
      channel_id = DS_LVAL (DS_U->channel_id);
    } else {
      assert (DS_U->message);
      assert (DS_U->message->to_id);
      assert (DS_U->message->to_id->magic == CODE_peer_channel);
      channel_id = DS_LVAL (DS_U->message->to_id->channel_id);
    }    

    bl_do_set_channel_pts (TLS, channel_id, DS_LVAL (DS_U->channel_pts));
  }
}

void tglu_work_updates (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }

  if (!check_only && do_skip_seq (TLS, DS_LVAL (DS_U->seq)) < 0) {
    return;
  }
  int i;
  if (DS_U->users) {
    for (i = 0; i < DS_LVAL (DS_U->users->cnt); i++) {
      tglf_fetch_alloc_user (TLS, DS_U->users->data[i]);    
    }
  }
  if (DS_U->chats) {
    for (i = 0; i < DS_LVAL (DS_U->chats->cnt); i++) {
      tglf_fetch_alloc_chat (TLS, DS_U->chats->data[i]);
    }
  }
  if (DS_U->updates) {
    for (i = 0; i < DS_LVAL (DS_U->updates->cnt); i++) {
      tglu_work_update (TLS, check_only, DS_U->updates->data[i]);
    }
  }

  if (check_only) { return; }
  bl_do_set_date (TLS, DS_LVAL (DS_U->date));
  bl_do_set_seq (TLS, DS_LVAL (DS_U->seq));
}

void tglu_work_updates_combined (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }

  if (!check_only && do_skip_seq (TLS, DS_LVAL (DS_U->seq_start)) < 0) {
    return;
  }
  
  int i;
  for (i = 0; i < DS_LVAL (DS_U->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_U->users->data[i]);    
  }
  for (i = 0; i < DS_LVAL (DS_U->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_U->chats->data[i]);
  }
  for (i = 0; i < DS_LVAL (DS_U->updates->cnt); i++) {
    tglu_work_update (TLS, check_only, DS_U->updates->data[i]);
  }

  if (check_only) { return; }
  bl_do_set_date (TLS, DS_LVAL (DS_U->date));
  bl_do_set_seq (TLS, DS_LVAL (DS_U->seq));
}

void tglu_work_update_short_message (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }

  if (!check_only && tgl_check_pts_diff (TLS, DS_LVAL (DS_U->pts), DS_LVAL (DS_U->pts_count)) <= 0) {
    return;
  }
  
  if (check_only > 0) { return; }
  
  //struct tgl_message *N = tgl_message_get (TLS, DS_LVAL (DS_U->id));
  //int new = (!N || !(N->flags & TGLMF_CREATED));
  
  struct tgl_message *M = tglf_fetch_alloc_message_short (TLS, DS_U);
  
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }
  assert (M);

  if (1) {
    bl_do_msg_update (TLS, &M->permanent_id);
  }
  
  if (check_only) { return; }
  bl_do_set_pts (TLS, DS_LVAL (DS_U->pts));
}

void tglu_work_update_short_chat_message (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }

  if (!check_only && tgl_check_pts_diff (TLS, DS_LVAL (DS_U->pts), DS_LVAL (DS_U->pts_count)) <= 0) {
    return;
  }
  
  if (check_only > 0) { return; }
  
  //struct tgl_message *N = tgl_message_get (TLS, DS_LVAL (DS_U->id));
  //int new = (!N || !(N->flags & TGLMF_CREATED));
  
  struct tgl_message *M = tglf_fetch_alloc_message_short_chat (TLS, DS_U);
  
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }
  
  assert (M);

  if (1) {
    bl_do_msg_update (TLS, &M->permanent_id);
  }

  if (check_only) { return; }
  bl_do_set_pts (TLS, DS_LVAL (DS_U->pts));
}

void tglu_work_updates_too_long (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }
  vlogprintf (E_NOTICE, "updates too long... Getting difference\n");
  if (check_only) { return; }
  tgl_do_get_difference (TLS, 0, 0, 0);
}

void tglu_work_update_short (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }
  tglu_work_update (TLS, check_only, DS_U->update);
}

void tglu_work_update_short_sent_message (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U, void *extra) {
  if (DS_U->pts) {
    assert (DS_U->pts_count);

    if (!check_only && tgl_check_pts_diff (TLS, DS_LVAL (DS_U->pts), DS_LVAL (DS_U->pts_count)) <= 0) {
      return;
    }
  }
  struct tgl_message *M = extra;

  if (!M) { return; }
  
  //long long random_id = M->permanent_id.id;
  tgl_message_id_t msg_id = M->permanent_id;
  msg_id.id = DS_LVAL (DS_U->id);
  bl_do_set_msg_id (TLS, &M->permanent_id, &msg_id);
  //tgls_insert_random2local (TLS, random_id, &msg_id);

  int f = DS_LVAL (DS_U->flags);

  unsigned flags = M->flags;
  if (f & 1) {
    flags |= TGLMF_UNREAD;
  }
  if (f & 2) {
    flags |= TGLMF_OUT;
  }
  if (f & 16) {
    flags |= TGLMF_MENTION;
  }

  bl_do_edit_message (TLS, &M->permanent_id, 
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, 0,
    DS_U->media,
    NULL,
    NULL,
    NULL, 
    NULL,
    flags);
 
  if (check_only) { return; }
  bl_do_msg_update (TLS, &M->permanent_id);
  
  if (DS_U->pts) {
    bl_do_set_pts (TLS, DS_LVAL (DS_U->pts));
  }
}

void tglu_work_any_updates (struct tgl_state *TLS, int check_only, struct tl_ds_updates *DS_U, void *extra) {
  if (check_only > 0 || (TLS->locks & TGL_LOCK_DIFF)) {
    return;
  }
  switch (DS_U->magic) {
  case CODE_updates_too_long:
    tglu_work_updates_too_long (TLS, check_only, DS_U);
    return;
  case CODE_update_short_message:
    tglu_work_update_short_message (TLS, check_only, DS_U);
    return;
  case CODE_update_short_chat_message:
    tglu_work_update_short_chat_message (TLS, check_only, DS_U);
    return;
  case CODE_update_short:
    tglu_work_update_short (TLS, check_only, DS_U);
    return;
  case CODE_updates_combined:
    tglu_work_updates_combined (TLS, check_only, DS_U);
    return;
  case CODE_updates:
    tglu_work_updates (TLS, check_only, DS_U);    
    return;
  case CODE_update_short_sent_message:
    tglu_work_update_short_sent_message (TLS, check_only, DS_U, extra);    
    return;
  default:
    assert (0);
  }
}

void tglu_work_any_updates_buf (struct tgl_state *TLS) {
  struct tl_ds_updates *DS_U = fetch_ds_type_updates (TYPE_TO_PARAM (updates));
  assert (DS_U);
  tglu_work_any_updates (TLS, 1, DS_U, NULL);
  tglu_work_any_updates (TLS, 0, DS_U, NULL);
  free_ds_type_updates (DS_U, TYPE_TO_PARAM (updates)); 
}

#define user_cmp(a,b) (tgl_get_peer_id ((a)->id) - tgl_get_peer_id ((b)->id))
DEFINE_TREE(user, struct tgl_user *,user_cmp,0)

static void notify_status (struct tgl_user *U, void *ex) {
  struct tgl_state *TLS = ex;
  if (TLS->callback.user_status_update) {
    TLS->callback.user_status_update (TLS, U);
  }
}

static void status_notify (struct tgl_state *TLS, void *arg) {
  tree_act_ex_user (TLS->online_updates, notify_status, TLS);
  tree_clear_user (TLS->online_updates);
  TLS->online_updates = NULL;
  TLS->timer_methods->free (TLS->online_updates_timer);
  TLS->online_updates_timer = NULL;
}

void tgl_insert_status_update (struct tgl_state *TLS, struct tgl_user *U) {
  if (!tree_lookup_user (TLS->online_updates, U)) {
    TLS->online_updates = tree_insert_user (TLS->online_updates, U, rand ());
  }
  if (!TLS->online_updates_timer) {
    TLS->online_updates_timer = TLS->timer_methods->alloc (TLS, status_notify, 0);
    TLS->timer_methods->insert (TLS->online_updates_timer, 0);
  }
}

static void user_expire (struct tgl_state *TLS, void *arg) {
  struct tgl_user *U = arg;
  TLS->timer_methods->free (U->status.ev);
  U->status.ev = 0;
  U->status.online = -1;
  U->status.when = tglt_get_double_time ();
  tgl_insert_status_update (TLS, U);
}

void tgl_insert_status_expire (struct tgl_state *TLS, struct tgl_user *U) {
  assert (!U->status.ev);
  U->status.ev = TLS->timer_methods->alloc (TLS, user_expire, U);
  TLS->timer_methods->insert (U->status.ev, U->status.when - tglt_get_double_time ()); 
}

void tgl_remove_status_expire (struct tgl_state *TLS, struct tgl_user *U) {
  TLS->timer_methods->free (U->status.ev);
  U->status.ev = 0;
}
