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
#include "config.h"
#endif

#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/utsname.h>
#endif

#include "mtproto-client.h"
#include "queries.h"
#include "tree.h"
#include "mtproto-common.h"
//#include "telegram.h"
#include "tgl-structures.h"
//#include "interface.h"
//#include "net.h"
#include "crypto/bn.h"
#include "crypto/rand.h"
#include "crypto/aes.h"
#include "crypto/sha.h"
#include "crypto/md5.h"

#include "no-preview.h"
#include "tgl-binlog.h"
#include "updates.h"
#include "auto.h"
#include "auto/auto-types.h"
#include "auto/auto-skip.h"
#include "auto/auto-free-ds.h"
#include "auto/auto-fetch-ds.h"
#include "auto/auto-print-ds.h"
#include "tgl.h"
#include "tg-mime-types.h"
#include "mtproto-utils.h"
#include "tgl-methods-in.h"
#include "tgl-queries.h"

#ifndef EPROTO
// BSD doesn't define EPROTO, even though it is POSIX:
// https://lists.freebsd.org/pipermail/freebsd-standards/2003-June/000124.html
#define EPROTO EIO
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


#ifndef O_BINARY
#define O_BINARY 0
#endif

static void out_peer_id (struct tgl_state *TLS, tgl_peer_id_t id);
static struct query_methods send_msgs_methods;

struct messages_send_extra {
  int multi;
  int count;
  tgl_message_id_t id;
  tgl_message_id_t *list;
};
#define QUERY_TIMEOUT 6.0

struct send_file {
  int fd;
  long long size;
  long long offset;
  int part_num;
  int part_size;
  long long id;
  long long thumb_id;
  tgl_peer_id_t to_id;
  int flags;
  char *file_name;
  int encr;
  //int avatar;
  tgl_peer_id_t avatar;
  int reply;
  unsigned char *iv;
  unsigned char *init_iv;
  unsigned char *key;
  int w;
  int h;
  int duration;
  char *caption;
  int channel;
};

#define memcmp8(a,b) memcmp ((a), (b), 8)
DEFINE_TREE (query, struct query *, memcmp8, 0) ;

static int mystreq1 (const char *a, const char *b, int l) {
  if ((int)strlen (a) != l) { return 1; }
  return memcmp (a, b, l);
}

/* {{{ COMMON */

struct query *tglq_query_get (struct tgl_state *TLS, long long id) {
  return tree_lookup_query (TLS->queries_tree, (void *)&id);
}

static int alarm_query (struct tgl_state *TLS, struct query *q) {
  assert (q);
  vlogprintf (E_DEBUG - 2, "Alarm query %" INT64_PRINTF_MODIFIER "d (type '%s')\n", q->msg_id, q->methods->name);

  TLS->timer_methods->insert (q->ev, q->methods->timeout ? q->methods->timeout : QUERY_TIMEOUT);

  if (q->session && q->session_id && q->DC && q->DC->sessions[0] == q->session && q->session->session_id == q->session_id) {
    clear_packet ();
    out_int (CODE_msg_container);
    out_int (1);
    out_long (q->msg_id);
    out_int (q->seq_no);
    out_int (4 * q->data_len);
    out_ints (q->data, q->data_len);

    tglmp_encrypt_send_message (TLS, q->session->c, packet_buffer, packet_ptr - packet_buffer, q->flags & QUERY_FORCE_SEND);
  } else {
    q->flags &= ~QUERY_ACK_RECEIVED;
    if (tree_lookup_query (TLS->queries_tree, q)) {
      TLS->queries_tree = tree_delete_query (TLS->queries_tree, q);
    }
    q->session = q->DC->sessions[0];
    long long old_id = q->msg_id;
    q->msg_id = tglmp_encrypt_send_message (TLS, q->session->c, q->data, q->data_len, (q->flags & QUERY_FORCE_SEND) | 1);
    vlogprintf (E_NOTICE, "Resent query #%" INT64_PRINTF_MODIFIER "d as #%" INT64_PRINTF_MODIFIER "d of size %d to DC %d\n", old_id, q->msg_id, 4 * q->data_len, q->DC->id);
    TLS->queries_tree = tree_insert_query (TLS->queries_tree, q, rand ());
    q->session_id = q->session->session_id;
    if (!(q->session->dc->flags & 4) && !(q->flags & QUERY_FORCE_SEND)) {
      q->session_id = 0;
    }
  }
  return 0;
}

void tglq_regen_query (struct tgl_state *TLS, long long id) {
  struct query *q = tglq_query_get (TLS, id);
  if (!q) { return; }
  q->flags &= ~QUERY_ACK_RECEIVED;

  if (!(q->session && q->session_id && q->DC && q->DC->sessions[0] == q->session && q->session->session_id == q->session_id)) {
    q->session_id = 0;
  } else {
    if (!(q->session->dc->flags & 4) && !(q->flags & QUERY_FORCE_SEND)) {
      q->session_id = 0;
    }
  }
  vlogprintf (E_NOTICE, "regen query %" INT64_PRINTF_MODIFIER "d\n", id);
  TLS->timer_methods->insert (q->ev, 0.001);
}

struct regen_tmp_struct {
  struct tgl_state *TLS;
  struct tgl_dc *DC;
  struct tgl_session *S;
};

void tglq_regen_query_from_old_session (struct query *q, void *ex) {
  struct regen_tmp_struct *T = ex;
  struct tgl_state *TLS = T->TLS;
  if (q->DC == T->DC) {
    if (!q->session || q->session_id != T->S->session_id || q->session != T->S) {
      q->session_id = 0;
      vlogprintf (E_NOTICE, "regen query from old session %" INT64_PRINTF_MODIFIER "d\n", q->msg_id);
      TLS->timer_methods->insert (q->ev, q->methods->timeout ? 0.001 : 0.1);
    }
  }
}

void tglq_regen_queries_from_old_session (struct tgl_state *TLS, struct tgl_dc *DC, struct tgl_session *S) {
  struct regen_tmp_struct T;
  T.TLS = TLS;
  T.DC = DC;
  T.S = S;
  tree_act_ex_query (TLS->queries_tree, tglq_regen_query_from_old_session, &T);
}

void tglq_query_restart (struct tgl_state *TLS, long long id) {
  struct query *q = tglq_query_get (TLS, id);
  if (q) {
    vlogprintf (E_NOTICE, "restarting query %" INT64_PRINTF_MODIFIER "d\n", id);
    TLS->timer_methods->remove (q->ev);
    alarm_query (TLS, q);
  }
}

static void alarm_query_gateway (struct tgl_state *TLS, void *arg) {
  alarm_query (TLS, arg);
}


struct query *tglq_send_query_ex (struct tgl_state *TLS, struct tgl_dc *DC, int ints, void *data, struct query_methods *methods, void *extra, void *callback, void *callback_extra, int flags) {
  assert (DC);
  assert (DC->auth_key_id);
  if (!DC->sessions[0]) {
    tglmp_dc_create_session (TLS, DC);
  }
  vlogprintf (E_DEBUG, "Sending query of size %d to DC %d\n", 4 * ints, DC->id);
  struct query *q = talloc0 (sizeof (*q));
  q->data_len = ints;
  q->data = talloc (4 * ints);
  memcpy (q->data, data, 4 * ints);
  q->msg_id = tglmp_encrypt_send_message (TLS, DC->sessions[0]->c, data, ints, 1 | (flags & QUERY_FORCE_SEND));
  q->session = DC->sessions[0];
  q->seq_no = q->session->seq_no - 1;
  q->session_id = q->session->session_id;
  if (!(DC->flags & 4) && !(flags & QUERY_FORCE_SEND)) {
    q->session_id = 0;
  }
  vlogprintf (E_DEBUG, "Msg_id is %" INT64_PRINTF_MODIFIER "d %p\n", q->msg_id, q);
  vlogprintf (E_NOTICE, "Sent query #%" INT64_PRINTF_MODIFIER "d of size %d to DC %d\n", q->msg_id, 4 * ints, DC->id);
  q->methods = methods;
  q->type = methods->type;
  q->DC = DC;
  q->flags = flags & QUERY_FORCE_SEND;
  if (TLS->queries_tree) {
    vlogprintf (E_DEBUG + 2, "%" INT64_PRINTF_MODIFIER "d %" INT64_PRINTF_MODIFIER "d\n", q->msg_id, TLS->queries_tree->x->msg_id);
  }
  TLS->queries_tree = tree_insert_query (TLS->queries_tree, q, rand ());

  q->ev = TLS->timer_methods->alloc (TLS, alarm_query_gateway, q);
  TLS->timer_methods->insert (q->ev, q->methods->timeout ? q->methods->timeout : QUERY_TIMEOUT);  

  q->extra = extra;
  q->callback = callback;
  q->callback_extra = callback_extra;
  TLS->active_queries ++;
  return q;
}

struct query *tglq_send_query (struct tgl_state *TLS, struct tgl_dc *DC, int ints, void *data, struct query_methods *methods, void *extra, void *callback, void *callback_extra) {
  return tglq_send_query_ex (TLS, DC, ints, data, methods, extra, callback, callback_extra, 0);
}

static int fail_on_error (struct tgl_state *TLS, struct query *q, int error_code, int l, const char *error) {
  vlogprintf (E_ERROR, "error #%d: %.*s\n", error_code, l, error);
  assert (0);
  return 0;
}

void tglq_query_ack (struct tgl_state *TLS, long long id) {
  struct query *q = tglq_query_get (TLS, id);
  if (q && !(q->flags & QUERY_ACK_RECEIVED)) {
    assert (q->msg_id == id);
    q->flags |= QUERY_ACK_RECEIVED;
    TLS->timer_methods->remove (q->ev);
  }
}

void tglq_query_delete (struct tgl_state *TLS, long long id) {
  struct query *q = tglq_query_get (TLS, id);
  if (!q) {
    return;
  }
  if (!(q->flags & QUERY_ACK_RECEIVED)) {
    TLS->timer_methods->remove (q->ev);
  }
  TLS->queries_tree = tree_delete_query (TLS->queries_tree, q);
  tfree (q->data, q->data_len * 4);
  TLS->timer_methods->free (q->ev);
  TLS->active_queries --;
}

static void resend_query_cb (struct tgl_state *TLS, void *_q, int success);

void tglq_free_query (struct query *q, void *extra) {
  struct tgl_state *TLS = extra;
  if (!(q->flags & QUERY_ACK_RECEIVED)) {
    TLS->timer_methods->remove (q->ev);
  }
  tfree (q->data, q->data_len * 4);
  TLS->timer_methods->free (q->ev);
}

void tglq_query_free_all (struct tgl_state *TLS) {
  tree_act_ex_query (TLS->queries_tree, tglq_free_query, TLS);
  TLS->queries_tree = tree_clear_query (TLS->queries_tree);
}

int tglq_query_error (struct tgl_state *TLS, long long id) {
  assert (fetch_int () == CODE_rpc_error);
  int error_code = fetch_int ();
  int error_len = prefetch_strlen ();
  char *error = fetch_str (error_len);
  struct query *q = tglq_query_get (TLS, id);
  if (!q) {
    vlogprintf (E_WARNING, "error for query '%s' #%" INT64_PRINTF_MODIFIER "d: #%d :%.*s\n", q->methods->name, id, error_code, error_len, error);
    vlogprintf (E_WARNING, "No such query\n");
  } else {
    if (!(q->flags & QUERY_ACK_RECEIVED)) {
      TLS->timer_methods->remove (q->ev);
    }
    TLS->queries_tree = tree_delete_query (TLS->queries_tree, q);
    int res = 0;

    int error_handled = 0;

    switch (error_code) {
    case 303:
      // migrate
      {
        int offset = -1;
        if (error_len >= 15 && !memcmp (error, "PHONE_MIGRATE_", 14)) {
          offset = 14;
        }
        if (error_len >= 17 && !memcmp (error, "NETWORK_MIGRATE_", 16)) {
          offset = 16;
        }
        if (error_len >= 14 && !memcmp (error, "USER_MIGRATE_", 13)) {
          offset = 13;
        }
        if (offset >= 0) {
          int i = 0;
          while (offset < error_len && error[offset] >= '0' && error[offset] <= '9') {
            i = i * 10 + error[offset] - '0';
            offset ++;
          }
          if (i > 0 && i < TGL_MAX_DC_NUM) {
            bl_do_set_working_dc (TLS, i);
            q->flags &= ~QUERY_ACK_RECEIVED;
            //q->session_id = 0;
            //struct tgl_dc *DC = q->DC;
            //if (!(DC->flags & 4) && !(q->flags & QUERY_FORCE_SEND)) {
            q->session_id = 0;
            //}
            q->DC = TLS->DC_working;
            TLS->timer_methods->insert (q->ev, 0);
            error_handled = 1;
            res = 1;
          }
        }
      }
      break;
    case 400:
      // nothing to handle
      // bad user input probably
      break;
    case 401:
      if (!mystreq1 ("SESSION_PASSWORD_NEEDED", error, error_len)) {
        if (!(TLS->locks & TGL_LOCK_PASSWORD)) {
          TLS->locks |= TGL_LOCK_PASSWORD;
          tgl_do_check_password (TLS, resend_query_cb, q);
        }
        res = 1;
        error_handled = 1;
      }
      break;
    case 403:
      // privacy violation
      break;
    case 404:
      // not found
      break;
    case 420:
      // flood
    case 500:
      // internal error
    default:
      // anything else. Treated as internal error
      {
        int wait;
        if (strncmp (error, "FLOOD_WAIT_", 11)) {
          if (error_code == 420) {
            vlogprintf (E_ERROR, "error = '%s'\n", error);
          }
          wait = 10;
        } else {
          wait = atoll (error + 11);
        }
        q->flags &= ~QUERY_ACK_RECEIVED;
        TLS->timer_methods->insert (q->ev, wait);
        struct tgl_dc *DC = q->DC;
        if (!(DC->flags & 4) && !(q->flags & QUERY_FORCE_SEND)) {
          q->session_id = 0;
        }
        error_handled = 1;
      }
      break;
    }

    if (error_handled) {
      vlogprintf (E_DEBUG - 2, "error for query #%" INT64_PRINTF_MODIFIER "d: #%d %.*s (HANDLED)\n", id, error_code, error_len, error);
    } else {
      vlogprintf (E_WARNING, "error for query '%s' #%" INT64_PRINTF_MODIFIER "d: #%d %.*s\n", q->methods->name, id, error_code, error_len, error);
      if (q->methods && q->methods->on_error) {
        res = q->methods->on_error (TLS, q, error_code, error_len, error);
      }
    }

    if (res <= 0) {
      tfree (q->data, q->data_len * 4);
      TLS->timer_methods->free (q->ev);
    }

    if (res == -11) {
      TLS->active_queries --;
      return -1;

    }
  }
  TLS->active_queries --;
  return 0;
}

#define MAX_PACKED_SIZE (1 << 24)
static int packed_buffer[MAX_PACKED_SIZE / 4];

int tglq_query_result (struct tgl_state *TLS, long long id) {
  vlogprintf (E_DEBUG, "result for query #%" INT64_PRINTF_MODIFIER "d. Size %ld bytes\n", id, (long)4 * (in_end - in_ptr));
  int op = prefetch_int ();
  int *end = 0;
  int *eend = 0;
  if (op == CODE_gzip_packed) {
    fetch_int ();
    int l = prefetch_strlen ();
    char *s = fetch_str (l);
    int total_out = tgl_inflate (s, l, packed_buffer, MAX_PACKED_SIZE);
    vlogprintf (E_DEBUG, "inflated %d bytes\n", total_out);
    end = in_ptr;
    eend = in_end;
    in_ptr = packed_buffer;
    in_end = in_ptr + total_out / 4;
  }
  struct query *q = tglq_query_get (TLS, id);
  if (!q) {
    vlogprintf (E_WARNING, "No such query\n");
    in_ptr = in_end;
  } else {
    if (!(q->flags & QUERY_ACK_RECEIVED)) {
      TLS->timer_methods->remove (q->ev);
    }
    TLS->queries_tree = tree_delete_query (TLS->queries_tree, q);
    if (q->methods && q->methods->on_answer) {
      assert (q->type);
      int *save = in_ptr;
      vlogprintf (E_DEBUG, "in_ptr = %p, end_ptr = %p\n", in_ptr, in_end);
      if (skip_type_any (q->type) < 0) {
        vlogprintf (E_ERROR, "Skipped %ld int out of %ld (type %s) (query type %s)\n", (long)(in_ptr - save), (long)(in_end - save), q->type->type->id, q->methods->name);
        vlogprintf (E_ERROR, "0x%08x 0x%08x 0x%08x 0x%08x\n", *(save - 1), *(save), *(save + 1), *(save + 2));
        assert (0);
      }

      assert (in_ptr == in_end);
      in_ptr = save;

      void *DS = fetch_ds_type_any (q->type);
      assert (DS);

      q->methods->on_answer (TLS, q, DS);
      free_ds_type_any (DS, q->type);

      assert (in_ptr == in_end);
    }
    tfree (q->data, 4 * q->data_len);
    TLS->timer_methods->free (q->ev);
    tfree (q, sizeof (*q));

  }
  if (end) {
    in_ptr = end;
    in_end = eend;
  }
  TLS->active_queries --;
  return 0;
}

static void out_random (int n) {
  assert (n <= 32);
  static char buf[32];
  tglt_secure_random (buf, n);
  out_cstring (buf, n);
}

int allow_send_linux_version;
void tgl_do_insert_header (struct tgl_state *TLS) {
  out_int (CODE_invoke_with_layer);
  out_int (TGL_SCHEME_LAYER);
  out_int (CODE_init_connection);
  out_int (TLS->app_id);
#ifndef WIN32
  if (allow_send_linux_version) {
    struct utsname st;
    uname (&st);
    out_string (st.machine);
    static char buf[4096];
    tsnprintf (buf, sizeof (buf) - 1, "%.999s %.999s %.999s", st.sysname, st.release, st.version);
    out_string (buf);
    tsnprintf (buf, sizeof (buf) - 1, "%s (TGL %s)", TLS->app_version, TGL_VERSION);
    out_string (buf);
    out_string ("En");
  } else {
    out_string ("x86");
    out_string ("Linux");
    static char buf[4096];
    tsnprintf (buf, sizeof (buf) - 1, "%s (TGL %s)", TLS->app_version, TGL_VERSION);
    out_string (buf);
    out_string ("en");
  }
#else
    out_string ("x86");
    out_string ("Windows");
    static char buf[4096];
    tsnprintf (buf, sizeof (buf) - 1, "%s (TGL %s)", TLS->app_version, TGL_VERSION);
    out_string (buf);
    out_string ("en");
#endif
}

void tgl_set_query_error (struct tgl_state *TLS, int error_code, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
void tgl_set_query_error (struct tgl_state *TLS, int error_code, const char *format, ...) {
  static char s[1001];

  va_list ap;
  va_start (ap, format);
  vsnprintf (s, 1000, format, ap);
  va_end (ap);

  if (TLS->error) {
    tfree_str (TLS->error);
  }
  TLS->error = tstrdup (s);
  TLS->error_code = error_code;
}
/* }}} */

/* {{{ Default on error */

static int q_void_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int))(q->callback))(TLS, q->callback_extra, 0);
  }
  return 0;
}

static int q_ptr_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, void *))(q->callback))(TLS, q->callback_extra, 0, NULL);
  }
  return 0;
}

static int q_list_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, int, void *))(q->callback))(TLS, q->callback_extra, 0, 0, NULL);
  }
  return 0;
}
/* }}} */

#include "queries-encrypted.c"

static void increase_ent (int *ent_size, int **ent, int s) {
  *ent = trealloc (*ent, (*ent_size) * 4, (*ent_size) * 4 + 4 * s);
  (*ent_size) +=s;
}

int utf8_len (const char *s, int len) {
  int i;
  int r = 0;
  for (i = 0; i < len; i++) {
    if ((s[i] & 0xc0) != 0x80) {
      r ++;
    }
  }
  return r;
}

static char *process_html_text (struct tgl_state *TLS, const char *text, int text_len, int *ent_size, int **ent) {
  char *new_text = talloc (2 * text_len + 1);
  int stpos[100];
  int sttype[100];
  int stp = 0;
  int p;
  int cur_p = 0;
  *ent = talloc (8);
  *ent_size = 2;
  (*ent)[0] = CODE_vector;
  (*ent)[1] = 0;
  int total = 0;
  for (p = 0; p < text_len; p++) {
    assert (cur_p <= 2 * text_len);
    if (text[p] == '<') {
      if (stp == 99) {
        tgl_set_query_error (TLS, EINVAL, "Too nested tags...");
        tfree (new_text, 2 * text_len + 1);
        return NULL;
      }
      int old_p = *ent_size;
      if (text_len - p >= 3 && !memcmp (text + p, "<b>", 3)) {
        increase_ent (ent_size, ent, 3);
        total ++;
        (*ent)[old_p] = CODE_message_entity_bold;
        (*ent)[old_p + 1] = utf8_len (new_text, cur_p);
        stpos[stp] = old_p + 2;
        sttype[stp] = 0;
        stp ++;
        p += 2;
        continue;
      }
      if (text_len - p >= 4 && !memcmp (text + p, "</b>", 4)) {
        if (stp == 0 || sttype[stp - 1]  != 0) {
          tgl_set_query_error (TLS, EINVAL, "Invalid tag nest");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }
        (*ent)[stpos[stp - 1]] = utf8_len (new_text, cur_p) - (*ent)[stpos[stp - 1] - 1];
        stp --;
        p += 3;
        continue;
      }
      if (text_len - p >= 3 && !memcmp (text + p, "<i>", 3)) {
        increase_ent (ent_size, ent, 3);
        total ++;
        (*ent)[old_p] = CODE_message_entity_italic;
        (*ent)[old_p + 1] = utf8_len (new_text, cur_p);
        stpos[stp] = old_p + 2;
        sttype[stp] = 1;
        stp ++;
        p += 2;
        continue;
      }
      if (text_len - p >= 4 && !memcmp (text + p, "</i>", 4)) {
        if (stp == 0 || sttype[stp - 1]  != 1) {
          tgl_set_query_error (TLS, EINVAL, "Invalid tag nest");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }
        (*ent)[stpos[stp - 1]] = utf8_len (new_text, cur_p) - (*ent)[stpos[stp - 1] - 1];
        stp --;
        p += 3;
        continue;
      }
      if (text_len - p >= 6 && !memcmp (text + p, "<code>", 6)) {
        increase_ent (ent_size, ent, 3);
        total ++;
        (*ent)[old_p] = CODE_message_entity_code;
        (*ent)[old_p + 1] = utf8_len (new_text, cur_p);
        stpos[stp] = old_p + 2;
        sttype[stp] = 2;
        stp ++;
        p += 5;
        continue;
      }
      if (text_len - p >= 7 && !memcmp (text + p, "</code>", 7)) {
        if (stp == 0 || sttype[stp - 1]  != 2) {
          tgl_set_query_error (TLS, EINVAL, "Invalid tag nest");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }
        (*ent)[stpos[stp - 1]] = utf8_len (new_text, cur_p) - (*ent)[stpos[stp - 1] - 1];
        stp --;
        p += 6;
        continue;
      }
      if (text_len - p >= 9 && !memcmp (text + p, "<a href=\"", 9)) {
        int pp = p + 9;
        while (pp < text_len && text[pp] != '"') {
          pp ++;
        }
        if (pp == text_len || pp == text_len - 1 || text[pp + 1] != '>') {
          tgl_set_query_error (TLS, EINVAL, "<a> tag did not close");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }
        int len = pp - p - 9;
        assert (len >= 0);
        if (len >= 250) {
          tgl_set_query_error (TLS, EINVAL, "too long link");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }

        increase_ent (ent_size, ent, 3 + (len + 1 + ((-len-1) & 3)) / 4);
        total ++;
        (*ent)[old_p] = CODE_message_entity_text_url;
        (*ent)[old_p + 1] = utf8_len (new_text, cur_p);
        stpos[stp] = old_p + 2;
        sttype[stp] = 3;
        stp ++;

        unsigned char *r = (void *)((*ent) + old_p + 3);
        r[0] = len;
        memcpy (r + 1, text + p + 9, len);
        memset (r + 1 + len, 0, (-len-1) & 3);

        p = pp + 1;
        continue;
      }
      if (text_len - p >= 4 && !memcmp (text + p, "</a>", 4)) {
        if (stp == 0 || sttype[stp - 1]  != 3) {
          tgl_set_query_error (TLS, EINVAL, "Invalid tag nest");
          tfree (new_text, 2 * text_len + 1);
          return NULL;
        }
        (*ent)[stpos[stp - 1]] = utf8_len (new_text, cur_p) - (*ent)[stpos[stp - 1] - 1];
        stp --;
        p += 3;
        continue;
      }
      if (text_len - p >= 4 && !memcmp (text + p, "<br>", 4)) {
        new_text[cur_p ++] = '\n';
        p += 3;
        continue;
      }
      tgl_set_query_error (TLS, EINVAL, "Unknown tag");
      tfree (new_text, 2 * text_len + 1);
      return NULL;
    } else if (text_len - p >= 4  && !memcmp (text + p, "&gt;", 4)) {
      p += 3;
      new_text[cur_p ++] = '>';
    } else if (text_len - p >= 4  && !memcmp (text + p, "&lt;", 4)) {
      p += 3;
      new_text[cur_p ++] = '<';
    } else if (text_len - p >= 5  && !memcmp (text + p, "&amp;", 5)) {
      p += 4;
      new_text[cur_p ++] = '&';
    } else if (text_len - p >= 6  && !memcmp (text + p, "&nbsp;", 6)) {
      p += 5;
      new_text[cur_p ++] = 0xc2;
      new_text[cur_p ++] = 0xa0;
    } else if (text_len - p >= 3  && text[p] == '&' && text[p + 1] == '#') {
      p += 2;
      int num = 0;
      int ok = 0;
      while (p < text_len) {
        if (text[p] >= '0' && text[p] <= '9') {
          num = num * 10 + text[p] - '0';
          if (num >= 127) {
            tgl_set_query_error (TLS, EINVAL, "Too big number in &-sequence");
            tfree (new_text, text_len + 1);
            return NULL;
          }
          p ++;
        } else if (text[p] == ';') {
          new_text[cur_p ++] =  num;
          ok = 1;
          break;
        } else {
          tgl_set_query_error (TLS, EINVAL, "Bad &-sequence");
          tfree (new_text, text_len + 1);
          return NULL;
        }
      }
      if (ok) { continue; }
      tgl_set_query_error (TLS, EINVAL, "Unterminated &-sequence");
      tfree (new_text, text_len + 1);
      return NULL;
    } else {
      new_text[cur_p ++] = text[p];
    }
  }
  if (stp != 0) {
    tgl_set_query_error (TLS, EINVAL, "Invalid tag nest");
    tfree (new_text, text_len + 1);
    return NULL;
  }
  (*ent)[1] = total;
  char *n = talloc (cur_p + 1);
  memcpy (n, new_text, cur_p);
  n[cur_p] = 0;
  tfree (new_text, 2 * text_len + 1);
  return n;
}

/* {{{ Get config */

static void fetch_dc_option (struct tgl_state *TLS, struct tl_ds_dc_option *DS_DO) {
  bl_do_dc_option (TLS, DS_LVAL (DS_DO->flags), DS_LVAL (DS_DO->id), NULL, 0, DS_STR (DS_DO->ip_address), DS_LVAL (DS_DO->port));
}

static int help_get_config_on_answer (struct tgl_state *TLS, struct query *q, void *DS) {
  struct tl_ds_config *DS_C = DS;

  int i;
  for (i = 0; i < DS_LVAL (DS_C->dc_options->cnt); i++) {
    fetch_dc_option (TLS, DS_C->dc_options->data[i]);
  }

  int max_chat_size = DS_LVAL (DS_C->chat_size_max);
  int max_bcast_size = 0;//DS_LVAL (DS_C->broadcast_size_max);
  vlogprintf (E_DEBUG, "chat_size = %d, bcast_size = %d\n", max_chat_size, max_bcast_size);

  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int))(q->callback))(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods help_get_config_methods  = {
  .on_answer = help_get_config_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(config),
  .name = "get config",
  .timeout = 1
};

void tgl_do_help_get_config (struct tgl_state *TLS, void (*callback)(struct tgl_state *,void *, int), void *callback_extra) {
  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_help_get_config);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &help_get_config_methods, 0, callback, callback_extra);
}

void tgl_do_help_get_config_dc (struct tgl_state *TLS, struct tgl_dc *D, void (*callback)(struct tgl_state *, void *, int), void *callback_extra) {
  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_help_get_config);
  tglq_send_query_ex (TLS, D, packet_ptr - packet_buffer, packet_buffer, &help_get_config_methods, 0, callback, callback_extra, 2);
}
/* }}} */

/* {{{ Send code */
static int send_code_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_auth_sent_code *DS_ASC = D;

  char *phone_code_hash = DS_STR_DUP (DS_ASC->phone_code_hash);
  int registered = DS_BVAL (DS_ASC->phone_registered);;

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, int, const char *))(q->callback)) (TLS, q->callback_extra, 1, registered, phone_code_hash);
  }
  tfree_str (phone_code_hash);
  return 0;
}

static struct query_methods send_code_methods  = {
  .on_answer = send_code_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM(auth_sent_code),
  .name = "send code"
};

void tgl_do_send_code (struct tgl_state *TLS, const char *phone, int phone_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int registered, const char *hash), void *callback_extra) {
  vlogprintf (E_DEBUG, "sending code to dc %d\n", TLS->dc_working_num);

  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_auth_send_code);
  out_cstring (phone, phone_len);
  out_int (0);
  out_int (TLS->app_id);
  out_string (TLS->app_hash);
  out_string ("en");

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_code_methods, NULL, callback, callback_extra);
}


static int phone_call_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int))(q->callback))(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods phone_call_methods  = {
  .on_answer = phone_call_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "phone call"
};

void tgl_do_phone_call (struct tgl_state *TLS, const char *phone, int phone_len, const char *hash, int hash_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  vlogprintf (E_DEBUG, "calling user\n");

  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_auth_send_call);
  out_cstring (phone, phone_len);
  out_cstring (hash, hash_len);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &phone_call_methods, NULL, callback, callback_extra);
}
/* }}} */

/* {{{ Sign in / Sign up */
static int sign_in_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_auth_authorization *DS_AA = D;
  //vlogprintf (E_DEBUG, "Expires in %d\n", DS_LVAL (DS_AA->expires));

  struct tgl_user *U = tglf_fetch_alloc_user (TLS, DS_AA->user);

  bl_do_dc_signed (TLS, TLS->DC_working->id);

  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, struct tgl_user *))q->callback) (TLS, q->callback_extra, 1, U);
  }

  return 0;
}

static int sign_in_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_user *))q->callback) (TLS, q->callback_extra, 0, NULL);
  }
  return 0;
}

static struct query_methods sign_in_methods  = {
  .on_answer = sign_in_on_answer,
  .on_error = sign_in_on_error,
  .type = TYPE_TO_PARAM(auth_authorization),
  .name = "sign in"
};

int tgl_do_send_code_result (struct tgl_state *TLS, const char *phone, int phone_len, const char *hash, int hash_len, const char *code, int code_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *Self), void *callback_extra) {
  clear_packet ();
  out_int (CODE_auth_sign_in);
  out_cstring (phone, phone_len);
  out_cstring (hash, hash_len);
  out_cstring (code, code_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &sign_in_methods, 0, callback, callback_extra);
  return 0;
}

int tgl_do_send_code_result_auth (struct tgl_state *TLS, const char *phone, int phone_len, const char *hash, int hash_len, const char *code, int code_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *Self), void *callback_extra) {
  clear_packet ();
  out_int (CODE_auth_sign_up);
  out_cstring (phone, phone_len);
  out_cstring (hash, hash_len);
  out_cstring (code, code_len);
  out_cstring (first_name, first_name_len);
  out_cstring (last_name, last_name_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &sign_in_methods, 0, callback, callback_extra);
  return 0;
}

int tgl_do_send_bot_auth (struct tgl_state *TLS, const char *code, int code_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *Self), void *callback_extra) {
  clear_packet ();
  out_int (CODE_auth_import_bot_authorization);
  out_int (0);
  out_int (TLS->app_id);
  out_string (TLS->app_hash);
  out_cstring (code, code_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &sign_in_methods, 0, callback, callback_extra);
  return 0;
}
/* }}} */

/* {{{ Get contacts */
static int get_contacts_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_contacts_contacts *DS_CC = D;

  int n = DS_CC->users ? DS_LVAL (DS_CC->users->cnt) : 0;

  int i;
  struct tgl_user **list = talloc (sizeof (void *) * n);
  for (i = 0; i < n; i++) {
    list[i] = tglf_fetch_alloc_user (TLS, DS_CC->users->data[i]);
  }
  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, int, struct tgl_user **))q->callback) (TLS, q->callback_extra, 1, n, list);
  }
  tfree (list, sizeof (void *) * n);
  return 0;
}

static struct query_methods get_contacts_methods = {
  .on_answer = get_contacts_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM(contacts_contacts),
  .name = "get contacts"
};


void tgl_do_update_contact_list (struct tgl_state *TLS, void (*callback) (struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *contacts[]), void *callback_extra) {
  clear_packet ();
  out_int (CODE_contacts_get_contacts);
  out_string ("");
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_contacts_methods, NULL, callback, callback_extra);
}
/* }}} */

/* {{{ Send msg (plain text) */
static int msg_send_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_updates *DS_U = D;

  tgl_message_id_t id;
  id.peer_type = TGL_PEER_RANDOM_ID;
  id.id = *(long long *)q->extra;
  tfree (q->extra, 8);

  struct tgl_message *M = tgl_message_get (TLS, &id);

  if (M && M->permanent_id.id == id.id) {
    tglu_work_any_updates (TLS, 1, DS_U, M);
    tglu_work_any_updates (TLS, 0, DS_U, M);
  } else {
    tglu_work_any_updates (TLS, 1, DS_U, NULL);
    tglu_work_any_updates (TLS, 0, DS_U, NULL);
  }

  M = tgl_message_get (TLS, &id);

  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 1, M);
  }
  return 0;
}

static int msg_send_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  tgl_message_id_t id;
  id.peer_type = TGL_PEER_RANDOM_ID;
  id.id = *(long long *)q->extra;
  tfree (q->extra, 8);

  struct tgl_message *M = tgl_message_get (TLS, &id);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 0, M);
  }
  if (M) {
    bl_do_message_delete (TLS, &M->permanent_id);
  }
  return 0;
}

static struct query_methods msg_send_methods = {
  .on_answer = msg_send_on_answer,
  .on_error = msg_send_on_error,
  .type = TYPE_TO_PARAM(updates),
  .name = "send message"
};

void tgl_do_send_msg (struct tgl_state *TLS, struct tgl_message *M, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (tgl_get_peer_type (M->to_id) == TGL_PEER_ENCR_CHAT) {
    tgl_do_send_encr_msg (TLS, M, callback, callback_extra);
    return;
  }
  clear_packet ();
  out_int (CODE_messages_send_message);

  unsigned f = ((M->flags & TGLMF_DISABLE_PREVIEW) ? 2 : 0) | (M->reply_id ? 1 : 0) | (M->reply_markup ? 4 : 0) | (M->entities_num > 0 ? 8 : 0);
  if (tgl_get_peer_type (M->from_id) == TGL_PEER_CHANNEL) {
    f |= 16;
  }
  out_int (f);
  out_peer_id (TLS, M->to_id);
  if (M->reply_id) {
    out_int (M->reply_id);
  }
  out_cstring (M->message, M->message_len);
  out_long (M->permanent_id.id);
  long long *x = talloc (8);
  *x = M->permanent_id.id;

  if (M->reply_markup) {
    if (M->reply_markup->rows) {
      out_int (CODE_reply_keyboard_markup);
      out_int (M->reply_markup->flags);
      out_int (CODE_vector);
      out_int (M->reply_markup->rows);
      int i;
      for (i = 0; i < M->reply_markup->rows; i++) {
        out_int (CODE_keyboard_button_row);
        out_int (CODE_vector);
        out_int (M->reply_markup->row_start[i + 1] - M->reply_markup->row_start[i]);
        int j;
        for (j = 0; j < M->reply_markup->row_start[i + 1] - M->reply_markup->row_start[i]; j++) {
          out_int (CODE_keyboard_button);
          out_string (M->reply_markup->buttons[j + M->reply_markup->row_start[i]]);
        }
      }
    } else {
      out_int (CODE_reply_keyboard_hide);
    }
  }

  if (M->entities_num > 0) {
    out_int (CODE_vector);
    out_int (M->entities_num);
    int i;
    for (i = 0; i < M->entities_num; i++) {
      struct tgl_message_entity *E = &M->entities[i];
      switch (E->type) {
      case tgl_message_entity_bold:
        out_int (CODE_message_entity_bold);
        out_int (E->start);
        out_int (E->length);
        break;
      case tgl_message_entity_italic:
        out_int (CODE_message_entity_italic);
        out_int (E->start);
        out_int (E->length);
        break;
      case tgl_message_entity_code:
        out_int (CODE_message_entity_code);
        out_int (E->start);
        out_int (E->length);
        break;
      case tgl_message_entity_text_url:
        out_int (CODE_message_entity_text_url);
        out_int (E->start);
        out_int (E->length);
        out_string (E->extra);
        break;
      default:
        assert (0);
      }
    }
  }

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &msg_send_methods, x, callback, callback_extra);
}

void tgl_do_send_message (struct tgl_state *TLS, tgl_peer_id_t peer_id, const char *text, int text_len, unsigned long long flags, struct tl_ds_reply_markup *reply_markup, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (tgl_get_peer_type (peer_id) == TGL_PEER_ENCR_CHAT) {
    tgl_peer_t *P = tgl_peer_get (TLS, peer_id);
    if (!P) {
      tgl_set_query_error (TLS, EINVAL, "unknown secret chat");
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
      return;
    }
    if (P->encr_chat.state != sc_ok) {
      tgl_set_query_error (TLS, EINVAL, "secret chat not in ok state");
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
      return;
    }
  }

  int date = time (0);

  struct tgl_message_id id = tgl_peer_id_to_random_msg_id (peer_id);

  if (tgl_get_peer_type (peer_id) != TGL_PEER_ENCR_CHAT) {
    int reply = (flags >> 32);
    int disable_preview = flags & TGL_SEND_MSG_FLAG_DISABLE_PREVIEW;
    if (!(flags & TGL_SEND_MSG_FLAG_ENABLE_PREVIEW) && TLS->disable_link_preview) {
      disable_preview = 1;
    }
    if (disable_preview) {
      disable_preview = TGLMF_DISABLE_PREVIEW;
    }
    struct tl_ds_message_media TDSM;
    TDSM.magic = CODE_message_media_empty;

    tgl_peer_id_t from_id;
    if (flags & TGLMF_POST_AS_CHANNEL) {
      from_id = peer_id;
    } else {
      from_id = TLS->our_id;
    }

    struct tl_ds_vector *EN = NULL;
    char *new_text = NULL;

    if (flags & TGLMF_HTML) {
      int ent_size = 0;
      int *ent = NULL;
      text = new_text = process_html_text (TLS, text, text_len, &ent_size, &ent);
      if (!text) {
        tfree (ent, ent_size);
        if (callback) {
          callback (TLS, callback_extra, 0, 0);
        }
        return;    
      }
      text_len = strlen (new_text);
      int *save_ptr = in_ptr;
      int *save_end = in_end;
      in_ptr = ent;
      in_end = ent + ent_size;
      EN = fetch_ds_type_any (TYPE_TO_PARAM_1 (vector, TYPE_TO_PARAM (message_entity)));
      assert (EN);
      vlogprintf (E_DEBUG, "in_ptr = %p, in_end = %p\n", in_ptr, in_end);
      assert (in_ptr == in_end);
      in_ptr = save_ptr;
      in_end = save_end;
      tfree (ent, 4 * ent_size);
    }

      
    bl_do_edit_message (TLS, &id, &from_id, &peer_id, NULL, NULL, &date, text, text_len, &TDSM, NULL, reply ? &reply : NULL, reply_markup, EN, TGLMF_UNREAD | TGLMF_OUT | TGLMF_PENDING | TGLMF_CREATE | TGLMF_CREATED | TGLMF_SESSION_OUTBOUND | disable_preview);
    
    if (flags & TGLMF_HTML) {
      tfree_str (new_text);
      free_ds_type_any (EN, TYPE_TO_PARAM_1 (vector, TYPE_TO_PARAM (message_entity)));
    }
  } else {
    struct tl_ds_decrypted_message_media TDSM;
    TDSM.magic = CODE_decrypted_message_media_empty;

    tgl_peer_id_t from_id = TLS->our_id;
    bl_do_edit_message_encr (TLS, &id, &from_id, &peer_id, &date, text, text_len, &TDSM, NULL, NULL, TGLMF_UNREAD | TGLMF_OUT | TGLMF_PENDING | TGLMF_CREATE | TGLMF_CREATED | TGLMF_SESSION_OUTBOUND | TGLMF_ENCRYPTED);
  }

  struct tgl_message *M = tgl_message_get (TLS, &id);
  assert (M);
  tgl_do_send_msg (TLS, M, callback, callback_extra);
}

void tgl_do_reply_message (struct tgl_state *TLS, tgl_message_id_t *_reply_id, const char *text, int text_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t reply_id = *_reply_id;
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    reply_id = tgl_convert_temp_msg_id (TLS, reply_id);
  }
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (reply_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not reply on message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  tgl_peer_id_t peer_id = tgl_msg_id_to_peer_id (reply_id);

  tgl_do_send_message (TLS, peer_id, text, text_len, flags | TGL_SEND_MSG_FLAG_REPLY (reply_id.id), NULL, callback, callback_extra);
}
/* }}} */

/* {{{ Send text file */
void tgl_do_send_text (struct tgl_state *TLS, tgl_peer_id_t id, const char *file_name, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  int fd = open (file_name, O_RDONLY | O_BINARY);
  if (fd < 0) {
    tgl_set_query_error (TLS, EBADF, "Can not open file: %s", strerror(errno));
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }
  static char buf[(1 << 20) + 1];
  int x = read (fd, buf, (1 << 20) + 1);
  if (x < 0) {
    tgl_set_query_error (TLS, EBADF, "Can not read from file: %s", strerror(errno));
    close (fd);
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }

  assert (x >= 0);
  close (fd);
  if (x == (1 << 20) + 1) {
    tgl_set_query_error (TLS, E2BIG, "text file is too big");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
  } else {
    tgl_do_send_message (TLS, id, buf, x, flags, NULL, callback, callback_extra);
  }
}

void tgl_do_reply_text (struct tgl_state *TLS, tgl_message_id_t *_reply_id, const char *file_name, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t reply_id = *_reply_id;
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    reply_id = tgl_convert_temp_msg_id (TLS, reply_id);
  }
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (reply_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not reply on message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  tgl_peer_id_t peer_id = tgl_msg_id_to_peer_id (reply_id);

  tgl_do_send_text (TLS, peer_id, file_name, flags | TGL_SEND_MSG_FLAG_REPLY (reply_id.id), callback, callback_extra);
}
/* }}} */

/* {{{ Mark read */

struct mark_read_extra {
  tgl_peer_id_t id;
  int max_id;
};

void tgl_do_messages_mark_read (struct tgl_state *TLS, tgl_peer_id_t id, int max_id, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int), void *callback_extra);

static int mark_read_channels_on_receive (struct tgl_state *TLS, struct query *q, void *D) {
  struct mark_read_extra *E = q->extra;

  bl_do_channel (TLS, tgl_get_peer_id (E->id), NULL, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL,
    &E->max_id, TGL_FLAGS_UNCHANGED);
  
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  tfree (E, sizeof (*E));
  return 0;
}

static int mark_read_on_receive (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_affected_messages *DS_MAM = D;

  int r = tgl_check_pts_diff (TLS, DS_LVAL (DS_MAM->pts), DS_LVAL (DS_MAM->pts_count));

  if (r > 0) {
    bl_do_set_pts (TLS, DS_LVAL (DS_MAM->pts));
  }

  struct mark_read_extra *E = q->extra;

  if (tgl_get_peer_type (E->id) == TGL_PEER_USER) {
    bl_do_user (TLS, tgl_get_peer_id (E->id), NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, &E->max_id, NULL, NULL, TGL_FLAGS_UNCHANGED);
  } else {
    assert (tgl_get_peer_type (E->id) == TGL_PEER_CHAT);
    bl_do_chat (TLS, tgl_get_peer_id (E->id), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &E->max_id, NULL, TGL_FLAGS_UNCHANGED);
  }
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  
  tfree (E, sizeof (*E));
  return 0;
}

static int mark_read_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct mark_read_extra *E = q->extra;
  tfree (E, sizeof (*E));
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
  }
  return 0;
}

static struct query_methods mark_read_methods = {
  .on_answer = mark_read_on_receive,
  .on_error = mark_read_on_error,
  .type = TYPE_TO_PARAM(messages_affected_messages),
  .name = "mark read"
};

static struct query_methods mark_read_channels_methods = {
  .on_answer = mark_read_channels_on_receive,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "mark read (channels)"
};

void tgl_do_messages_mark_read (struct tgl_state *TLS, tgl_peer_id_t id, int max_id, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int), void *callback_extra) {
  if (TLS->is_bot) { return; }
  if (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT) {
    tgl_do_mark_read (TLS, id, callback, callback_extra);
    return;
  }
  clear_packet ();
  if (tgl_get_peer_type (id) != TGL_PEER_CHANNEL) {
    out_int (CODE_messages_read_history);
    out_peer_id (TLS, id);
    out_int (max_id);
    //out_int (offset);

    struct mark_read_extra *E = talloc (sizeof (*E));
    E->id = id;
    E->max_id = max_id;

    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &mark_read_methods, E, callback, callback_extra);
  } else {
    out_int (CODE_channels_read_history);

    out_int (CODE_input_channel);
    out_int (tgl_get_peer_id (id));
    out_long (id.access_hash);
    
    out_int (max_id);

    struct mark_read_extra *E = talloc (sizeof (*E));
    E->id = id;
    E->max_id = max_id;
    
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &mark_read_channels_methods, E, callback, callback_extra);
  }
}

void tgl_do_mark_read (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  if (tgl_get_peer_type (id) == TGL_PEER_USER || tgl_get_peer_type (id) == TGL_PEER_CHAT || tgl_get_peer_type (id) == TGL_PEER_CHANNEL) {
    tgl_do_messages_mark_read (TLS, id, 0, 0, callback, callback_extra);
    return;
  }
  assert (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT);
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P) {
    tgl_set_query_error (TLS, EINVAL, "unknown secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  if (P->last) {
    tgl_do_messages_mark_read_encr (TLS, id, P->encr_chat.access_hash, P->last->date, callback, callback_extra);
  } else {
    tgl_do_messages_mark_read_encr (TLS, id, P->encr_chat.access_hash, time (0) - 10, callback, callback_extra);
  }
}
/* }}} */

/* {{{ Get history */
struct get_history_extra {
  struct tgl_message **ML;
  int list_offset;
  int list_size;
  tgl_peer_id_t id;
  int limit;
  int offset;
  int max_id;
};

static void _tgl_do_get_history (struct tgl_state *TLS, struct get_history_extra *E, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra);


static int get_history_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_messages *DS_MM = D;

  int i;
  for (i = 0; i < DS_LVAL (DS_MM->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_MM->chats->data[i]);
  }

  for (i = 0; i < DS_LVAL (DS_MM->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_MM->users->data[i]);
  }

  struct get_history_extra *E = q->extra;

  int n = DS_LVAL (DS_MM->messages->cnt);

  if (E->list_size - E->list_offset < n) {
    int new_list_size = 2 * E->list_size;
    if (new_list_size - E->list_offset < n) {
      new_list_size = n + E->list_offset;
    }
    E->ML = trealloc (E->ML, E->list_size * sizeof (void *), new_list_size * sizeof (void *));
    assert (E->ML);
    E->list_size = new_list_size;
  }

  for (i = 0; i < n; i++) {
    E->ML[i + E->list_offset] = tglf_fetch_alloc_message (TLS, DS_MM->messages->data[i], NULL);
  }
  E->list_offset += n;
  E->offset += n;
  E->limit -= n;

  int count = DS_LVAL (DS_MM->count);
  if (count >= 0 && E->limit + E->offset >= count) {
    E->limit = count - E->offset;
    if (E->limit < 0) { E->limit = 0; }
  }
  assert (E->limit >= 0);


  if (E->limit <= 0 || DS_MM->magic == CODE_messages_messages || DS_MM->magic == CODE_messages_channel_messages) {
    if (q->callback) {
      ((void (*)(struct tgl_state *TLS, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 1, E->list_offset, E->ML);
    }
    /*if (E->list_offset > 0) {
      tgl_do_messages_mark_read (TLS, E->id, E->ML[0]->id, 0, 0, 0);
    }*/

    tfree (E->ML, sizeof (void *) * E->list_size);
    tfree (E, sizeof (*E));
  } else {
    E->offset = 0;
    E->max_id = E->ML[E->list_offset - 1]->permanent_id.id;
    _tgl_do_get_history (TLS, E, q->callback, q->callback_extra);
  }
  return 0;
}

static int get_history_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct get_history_extra *E = q->extra;
  tfree (E->ML, sizeof (void *) * E->list_size);
  tfree (E, sizeof (*E));

  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 0, 0, NULL);
  }
  return 0;
}

static struct query_methods get_history_methods = {
  .on_answer = get_history_on_answer,
  .on_error = get_history_on_error,
  .type = TYPE_TO_PARAM(messages_messages),
  .name = "get history"
};

void tgl_do_get_local_history (struct tgl_state *TLS, tgl_peer_id_t id, int offset, int limit, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra) {
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  if (!P || !P->last) {
    tgl_set_query_error (TLS, EINVAL, "unknown peer");
    if (callback) {
      callback (TLS, callback_extra, 0, 0, 0);
    }
    return;
  }
  struct tgl_message *M = P->last;
  int count = 1;
  assert (!M->prev);
  while (count < limit + offset && M->next) {
    M = M->next;
    count ++;
  }
  if (count <= offset) {
    if (callback) {
      callback (TLS, callback_extra, 1, 0, 0);
    }
    return;
  }
  struct tgl_message **ML = talloc (sizeof (void *) * (count - offset));
  M = P->last;
  ML[0] = M;
  count = 1;
  while (count < limit && M->next) {
    M = M->next;
    if (count >= offset) {
      ML[count - offset] = M;
    }
    count ++;
  }

  if (callback) {
    callback (TLS, callback_extra, 1, count - offset, ML);
  }
  tfree (ML, sizeof (void *) * (count) - offset);
}

static void _tgl_do_get_history (struct tgl_state *TLS, struct get_history_extra *E, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra) {
  clear_packet ();
  tgl_peer_t *C = tgl_peer_get (TLS, E->id);
  if (tgl_get_peer_type (E->id) != TGL_PEER_CHANNEL || (C && (C->flags & TGLCHF_MEGAGROUP))) {
    out_int (CODE_messages_get_history);
    out_peer_id (TLS, E->id);
  } else {    
    out_int (CODE_channels_get_important_history);
    
    out_int (CODE_input_channel);
    out_int (tgl_get_peer_id (E->id));
    out_long (E->id.access_hash);
  }
  out_int (E->max_id);
  out_int (E->offset);
  out_int (E->limit);
  out_int (0);
  out_int (0);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_history_methods, E, callback, callback_extra);
}

void tgl_do_get_history (struct tgl_state *TLS, tgl_peer_id_t id, int offset, int limit, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra) {
  if (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT || offline_mode) {
    tgl_do_get_local_history (TLS, id, offset, limit, callback, callback_extra);
    //tgl_do_mark_read (TLS, id, 0, 0);
    return;
  }
  struct get_history_extra *E = talloc0 (sizeof (*E));
  E->id = id;
  E->limit = limit;
  E->offset = offset;
  _tgl_do_get_history (TLS, E, callback, callback_extra);
}
/* }}} */

/* {{{ Get dialogs */
struct get_dialogs_extra {
  tgl_peer_id_t *PL;
  int *UC;
  tgl_message_id_t **LM;
  tgl_message_id_t *LMD;
  int *LRM;

  int list_offset;
  int list_size;
  int limit;
  int offset;
  int offset_date;
  int max_id;
  tgl_peer_id_t offset_peer;

  int channels;
};

static void _tgl_do_get_dialog_list (struct tgl_state *TLS, struct get_dialogs_extra *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra);

static int get_dialogs_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_dialogs *DS_MD = D;

  struct get_dialogs_extra *E = q->extra;

  int dl_size = DS_LVAL (DS_MD->dialogs->cnt);

  int i;
  for (i = 0; i < DS_LVAL (DS_MD->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_MD->chats->data[i]);
  }

  for (i = 0; i < DS_LVAL (DS_MD->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_MD->users->data[i]);
  }

  if (E->list_offset + dl_size > E->list_size) {
    int new_list_size = E->list_size * 2;
    if (new_list_size < E->list_offset + dl_size) {
      new_list_size = E->list_offset + dl_size;
    }

    E->PL = trealloc (E->PL, E->list_size * sizeof (tgl_peer_id_t), new_list_size * sizeof (tgl_peer_id_t));
    assert (E->PL);
    E->UC = trealloc (E->UC, E->list_size * sizeof (int), new_list_size * sizeof (int));
    assert (E->UC);
    E->LM = trealloc (E->LM, E->list_size * sizeof (void *), new_list_size * sizeof (void *));
    assert (E->LM);
    E->LMD = trealloc (E->LMD, E->list_size * sizeof (tgl_message_id_t), new_list_size * sizeof (tgl_message_id_t));
    assert (E->LMD);
    E->LRM = trealloc (E->LRM, E->list_size * sizeof (int), new_list_size * sizeof (int));
    assert (E->LRM);

    E->list_size = new_list_size;

    int i;
    for (i = 0; i < E->list_offset; i++) {
      E->LM[i] = &E->LMD[i];
    }
  }

  for (i = 0; i < dl_size; i++) {
    struct tl_ds_dialog *DS_D = DS_MD->dialogs->data[i];
    tgl_peer_t *P = tgl_peer_get (TLS, tglf_fetch_peer_id (TLS, DS_D->peer));
    assert (P);
    E->PL[E->list_offset + i] = P->id;
    E->LMD[E->list_offset + i] = tgl_peer_id_to_msg_id (E->PL[E->list_offset + i], DS_LVAL (DS_D->top_message));
    E->LM[E->list_offset + i] = &E->LMD[E->list_offset + i];
    E->UC[E->list_offset + i] = DS_LVAL (DS_D->unread_count);
    E->LRM[E->list_offset + i] = DS_LVAL (DS_D->read_inbox_max_id);
  }
  E->list_offset += dl_size;

  for (i = 0; i < DS_LVAL (DS_MD->messages->cnt); i++) {
    tglf_fetch_alloc_message (TLS, DS_MD->messages->data[i], NULL);
  }

  vlogprintf (E_DEBUG, "dl_size = %d, total = %d\n", dl_size, E->list_offset);
  if (dl_size && E->list_offset < E->limit && DS_MD->magic == CODE_messages_dialogs_slice && E->list_offset < DS_LVAL (DS_MD->count)) {
    E->offset += dl_size;
    if (E->list_offset > 0) {
      E->offset_peer = E->PL[E->list_offset - 1];
    
      int p = E->list_offset - 1;
      while (p >= 0) {
        struct tgl_message *M = tgl_message_get (TLS, E->LM[p]);
        if (M) {
          E->offset_date = M->date;
          break;
        }
        p --;
      }
    }
    _tgl_do_get_dialog_list (TLS, E, q->callback, q->callback_extra);
  } else {
    if (q->callback) {
      ((void (*)(struct tgl_state *TLS, void *, int, int, tgl_peer_id_t *, tgl_message_id_t **, int *))q->callback) (TLS, q->callback_extra, 1, E->list_offset, E->PL, E->LM, E->UC);
    }
    tfree (E->PL, sizeof (tgl_peer_id_t) * E->list_size);
    tfree (E->UC, 4 * E->list_size);
    tfree (E->LM, sizeof (void *) * E->list_size);
    tfree (E->LMD, sizeof (tgl_message_id_t) * E->list_size);
    tfree (E->LRM, 4 * E->list_size);
    tfree (E, sizeof (*E));
  }

  return 0;
}

static int get_dialogs_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct get_dialogs_extra *E = q->extra;
  tfree (E->PL, sizeof (tgl_peer_id_t) * E->list_size);
  tfree (E->UC, 4 * E->list_size);
  tfree (E->LM, sizeof (void *) * E->list_size);
  tfree (E->LMD, sizeof (tgl_message_id_t) * E->list_size);
  tfree (E->LRM, 4 * E->list_size);
  tfree (E, sizeof (*E));
  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, int, tgl_peer_id_t *, tgl_message_id_t **, int *))q->callback) (TLS, q->callback_extra, 0, 0, NULL, NULL, NULL);
  }
  return 0;
}

static struct query_methods get_dialogs_methods = {
  .on_answer = get_dialogs_on_answer,
  .on_error = get_dialogs_on_error,
  .type = TYPE_TO_PARAM(messages_dialogs),
  .name = "get dialogs"
};

static void _tgl_do_get_dialog_list (struct tgl_state *TLS, struct get_dialogs_extra *E,  void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra) {
  clear_packet ();
  if (E->channels) {
    out_int (CODE_channels_get_dialogs);
    out_int (E->offset);
    out_int (E->limit - E->list_offset);
  } else {
    out_int (CODE_messages_get_dialogs);
    out_int (E->offset_date);
    out_int (E->offset);
    //out_int (0);
    if (E->offset_peer.peer_type) {
      out_peer_id (TLS, E->offset_peer);
    } else {
      out_int (CODE_input_peer_empty);
    }
    out_int (E->limit - E->list_offset);
  }

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_dialogs_methods, E, callback, callback_extra);
}

void tgl_do_get_dialog_list (struct tgl_state *TLS, int limit, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra) {
  struct get_dialogs_extra *E = talloc0 (sizeof (*E));
  E->limit = limit;
  E->offset = offset;
  E->channels = 0;
  _tgl_do_get_dialog_list (TLS, E, callback, callback_extra);
}

void tgl_do_get_channels_dialog_list (struct tgl_state *TLS, int limit, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra) {
  struct get_dialogs_extra *E = talloc0 (sizeof (*E));
  E->limit = limit;
  E->offset = offset;
  E->channels = 1;
  E->offset_date = 0;
  E->offset_peer.peer_type = 0;
  _tgl_do_get_dialog_list (TLS, E, callback, callback_extra);
}
/* }}} */

int allow_send_linux_version = 1;

/* {{{ Send document file */

static void out_peer_id (struct tgl_state *TLS, tgl_peer_id_t id) {
  switch (tgl_get_peer_type (id)) {
  case TGL_PEER_CHAT:
    out_int (CODE_input_peer_chat);
    out_int (tgl_get_peer_id (id));
    break;
  case TGL_PEER_USER:
    out_int (CODE_input_peer_user);
    out_int (tgl_get_peer_id (id));
    out_long (id.access_hash);
    break;
  case TGL_PEER_CHANNEL:
    out_int (CODE_input_peer_channel);
    out_int (tgl_get_peer_id (id));
    out_long (id.access_hash);
    break;
  default:
    assert (0);
  }
}

static void send_part (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra);
static int send_file_part_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  send_part (TLS, q->extra, q->callback, q->callback_extra);
  return 0;
}

static int set_photo_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static int send_file_part_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct send_file *f = q->extra;
  tfree_str (f->file_name);
  tfree_str (f->caption);
  if (!f->avatar.peer_type) {
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 0, 0);
    }
  } else {
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 0);
    }
  }
  tfree (f, sizeof (*f));
  return 0;
}

static struct query_methods send_file_part_methods = {
  .on_answer = send_file_part_on_answer,
  .on_error = send_file_part_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "send file part"
};

static struct query_methods set_photo_methods = {
  .on_answer = set_photo_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(photos_photo),
  .name = "set photo"
};

static void send_avatar_end (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra) {
  switch (tgl_get_peer_type (f->avatar)) {
  case TGL_PEER_CHAT:
    out_int (CODE_messages_edit_chat_photo);
    out_int (f->avatar.peer_id);
    out_int (CODE_input_chat_uploaded_photo);
    break;
  case TGL_PEER_USER:
    out_int (CODE_photos_upload_profile_photo);
    out_int (CODE_photos_upload_profile_photo);
    break;
  case TGL_PEER_CHANNEL:
    out_int (CODE_channels_edit_photo);
    out_int (CODE_input_channel);
    out_int (f->avatar.peer_id);
    out_long (f->avatar.access_hash);
    out_int (CODE_input_chat_uploaded_photo);
    break;
  default:
    assert (0);
  }

  if (f->size < (16 << 20)) {
    out_int (CODE_input_file);
  } else {
    out_int (CODE_input_file_big);
  }
  out_long (f->id);
  out_int (f->part_num);

  char *s = f->file_name + strlen (f->file_name);
  while (s >= f->file_name && *s != '/') { s --;}
  out_string (s + 1);
  if (f->size < (16 << 20)) {
    out_string ("");
  }

  if (f->avatar.peer_type != TGL_PEER_USER) {
    out_int (CODE_input_photo_crop_auto);
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, NULL, callback, callback_extra);
  } else {
    out_string ("profile photo");
    out_int (CODE_input_geo_point_empty);
    out_int (CODE_input_photo_crop_auto);
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_photo_methods, 0, callback, callback_extra);
  }
}


static void send_file_unencrypted_end (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra) {
  out_int (CODE_messages_send_media);
  out_int ((f->reply ? 1 : 0) | f->channel);
  out_peer_id (TLS, f->to_id);
  if (f->reply) {
    out_int (f->reply);
  }
  if (f->flags & TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO) {
    out_int (CODE_input_media_uploaded_photo);
  } else {
    if (f->thumb_id > 0) {
      out_int (CODE_input_media_uploaded_thumb_document);
    } else {
      out_int (CODE_input_media_uploaded_document);
    }
  }

  if (f->size < (16 << 20)) {
    out_int (CODE_input_file);
  } else {
    out_int (CODE_input_file_big);
  }

  out_long (f->id);
  out_int (f->part_num);
  char *s = f->file_name + strlen (f->file_name);
  while (s >= f->file_name && *s != '/') { s --;}
  out_string (s + 1);
  if (f->size < (16 << 20)) {
    out_string ("");
  }

  if (!(f->flags & TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO)) {
    out_string (tg_mime_by_filename (f->file_name));

    out_int (CODE_vector);
    if (f->flags & TGLDF_IMAGE) {
      if (f->flags & TGLDF_ANIMATED) {
        out_int (2);
        out_int (CODE_document_attribute_image_size);
        out_int (f->w);
        out_int (f->h);
        out_int (CODE_document_attribute_animated);
      } else {
        out_int (1);
        out_int (CODE_document_attribute_image_size);
        out_int (f->w);
        out_int (f->h);
      }
    } else if (f->flags & TGLDF_AUDIO) {
      out_int (2);
      out_int (CODE_document_attribute_audio);
      out_int (f->duration);
      out_string ("");
      out_string ("");
      out_int (CODE_document_attribute_filename);
      out_string (s + 1);
    } else if (f->flags & TGLDF_VIDEO) {
      out_int (2);
      out_int (CODE_document_attribute_video);
      out_int (f->duration);
      out_int (f->w);
      out_int (f->h);
      out_int (CODE_document_attribute_filename);
      out_string (s + 1);
    } else if (f->flags & TGLDF_STICKER) {
      out_int (1);
      out_int (CODE_document_attribute_sticker);
    } else {
      out_int (1);
      out_int (CODE_document_attribute_filename);
      out_string (s + 1);
    }

    if (f->thumb_id > 0) {
      out_int (CODE_input_file);
      out_long (f->thumb_id);
      out_int (1);
      out_string ("thumb.jpg");
      out_string ("");
    }
    
    out_string (f->caption ? f->caption : "");
  } else {
    out_string (f->caption ? f->caption : "");
  }


  struct messages_send_extra *E = talloc0 (sizeof (*E));
  E->id = tgl_peer_id_to_random_msg_id (f->to_id);
  out_long (E->id.id);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
  tfree_str (f->file_name);
  tfree_str (f->caption);
  tfree (f, sizeof (*f));
}

static void send_file_end (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra) {
  TLS->cur_uploaded_bytes -= f->size;
  TLS->cur_uploading_bytes -= f->size;
  clear_packet ();

  if (f->avatar.peer_id) {
    send_avatar_end (TLS, f, callback, callback_extra);
    return;
  }
  if (!f->encr) {
    send_file_unencrypted_end (TLS, f, callback, callback_extra);
    return;
  }
  send_file_encrypted_end (TLS, f, callback, callback_extra);
  return;
}

static void send_part (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra) {
  if (f->fd >= 0) {
    if (!f->part_num) {
      TLS->cur_uploading_bytes += f->size;
    }
    clear_packet ();
    if (f->size < (16 << 20)) {
      out_int (CODE_upload_save_file_part);
      out_long (f->id);
      out_int (f->part_num ++);
    } else {
      out_int (CODE_upload_save_big_file_part);
      out_long (f->id);
      out_int (f->part_num ++);
      out_int ((f->size + f->part_size - 1) / f->part_size);
    }
    static char buf[512 << 10];
    int x = read (f->fd, buf, f->part_size);
    assert (x > 0);
    f->offset += x;
    TLS->cur_uploaded_bytes += x;

    if (f->encr) {
      if (x & 15) {
        assert (f->offset == f->size);
        tglt_secure_random (buf + x, (-x) & 15);
        x = (x + 15) & ~15;
      }

      TGLC_aes_key aes_key;
      TGLC_aes_set_encrypt_key (f->key, 256, &aes_key);
      TGLC_aes_ige_encrypt ((void *)buf, (void *)buf, x, &aes_key, f->iv, 1);
      memset (&aes_key, 0, sizeof (aes_key));
    }
    out_cstring (buf, x);
    vlogprintf (E_DEBUG, "offset=%" INT64_PRINTF_MODIFIER "d size=%" INT64_PRINTF_MODIFIER "d\n", f->offset, f->size);
    if (f->offset == f->size) {
      close (f->fd);
      f->fd = -1;
    } else {
      assert (f->part_size == x);
    }
    //update_prompt ();
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_file_part_methods, f, callback, callback_extra);
  } else {
    send_file_end (TLS, f, callback, callback_extra);
  }
}

static void send_file_thumb (struct tgl_state *TLS, struct send_file *f, const void *thumb_data, int thumb_len, void *callback, void *callback_extra) {
  clear_packet ();
  tglt_secure_random (&f->thumb_id, 8);
  out_int (CODE_upload_save_file_part);
  out_long (f->thumb_id);
  out_int (0);
  out_cstring ((void *)thumb_data, thumb_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_file_part_methods, f, callback, callback_extra);
}


static void _tgl_do_send_photo (struct tgl_state *TLS, tgl_peer_id_t to_id, const char *file_name, tgl_peer_id_t avatar, int w, int h, int duration, const void *thumb_data, int thumb_len, const char *caption, int caption_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  int fd = open (file_name, O_RDONLY | O_BINARY);
  if (fd < 0) {
    tgl_set_query_error (TLS, EBADF, "Can not open file: %s", strerror(errno));
    if (!avatar.peer_id) {
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        ((void (*)(struct tgl_state *, void *, int))callback) (TLS, callback_extra, 0);
      }
    }
    return;
  }
  struct stat buf;
  fstat (fd, &buf);
  long long size = buf.st_size;
  if (size <= 0) {
    tgl_set_query_error (TLS, EBADF, "File is empty");
    close (fd);
    if (!avatar.peer_id) {
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        ((void (*)(struct tgl_state *, void *, int))callback) (TLS, callback_extra, 0);
      }
    }
    return;
  }
  struct send_file *f = talloc0 (sizeof (*f));
  f->fd = fd;
  f->size = size;
  f->offset = 0;
  f->part_num = 0;
  f->avatar = avatar;
  f->reply = flags >> 32;
  int tmp = ((size + 2999) / 3000);
  f->part_size = (1 << 14);
  while (f->part_size < tmp) {
    f->part_size *= 2;
  }
  f->flags = flags;
  f->channel = 0;
  if (flags & TGLMF_POST_AS_CHANNEL) {
    f->channel = 16;
  }

  if (f->part_size > (512 << 10)) {
    close (fd);
    tgl_set_query_error (TLS, E2BIG, "File is too big");
    tfree (f, sizeof (*f));
    if (!avatar.peer_id) {
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        ((void (*)(struct tgl_state *, void *, int))callback) (TLS, callback_extra, 0);
      }
    }
    return;
  }

  tglt_secure_random (&f->id, 8);
  f->to_id = to_id;
  f->flags = flags;
  f->file_name = tstrdup (file_name);
  f->w = w;
  f->h = h;
  f->duration = duration;
  f->caption = caption ? tstrdup (caption) : tstrdup ("");

  if (tgl_get_peer_type (f->to_id) == TGL_PEER_ENCR_CHAT) {
    f->encr = 1;
    f->iv = talloc (32);
    tglt_secure_random (f->iv, 32);
    f->init_iv = talloc (32);
    memcpy (f->init_iv, f->iv, 32);
    f->key = talloc (32);
    tglt_secure_random (f->key, 32);
  }

  if (!f->encr && f->flags != -1 && thumb_len > 0) {
    send_file_thumb (TLS, f, thumb_data, thumb_len, callback, callback_extra);
  } else {
    send_part (TLS, f, callback, callback_extra);
  }
}

void tgl_do_send_document (struct tgl_state *TLS, tgl_peer_id_t to_id, const char *file_name, const char *caption, int caption_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (flags & TGL_SEND_MSG_FLAG_DOCUMENT_AUTO) {
    char *mime_type = tg_mime_by_filename (file_name);
    if (strcmp (mime_type, "image/gif") == 0) {
      flags |= TGL_SEND_MSG_FLAG_DOCUMENT_ANIMATED;
    } else if (!memcmp (mime_type, "image/", 6)) {
      flags |= TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO;
    } else if (!memcmp (mime_type, "video/", 6)) {
      flags |= TGLDF_VIDEO;
    } else if (!memcmp (mime_type, "audio/", 6)) {
      flags |= TGLDF_AUDIO;
    }
  }
  tgl_peer_id_t x;
  x.peer_id = 0;
  _tgl_do_send_photo (TLS, to_id, file_name, x, 100, 100, 100, 0, 0, caption, caption_len, flags, callback, callback_extra);
}

void tgl_do_reply_document (struct tgl_state *TLS, tgl_message_id_t *_reply_id, const char *file_name, const char *caption, int caption_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t reply_id = *_reply_id;
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    reply_id = tgl_convert_temp_msg_id (TLS, reply_id);
  }
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (reply_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not reply on message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  tgl_peer_id_t peer_id = tgl_msg_id_to_peer_id (reply_id);

  tgl_do_send_document (TLS, peer_id, file_name, caption, caption_len, flags | TGL_SEND_MSG_FLAG_REPLY (reply_id.id), callback, callback_extra);
}

void tgl_do_set_chat_photo (struct tgl_state *TLS, tgl_peer_id_t chat_id, const char *file_name, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success), void *callback_extra) {
  assert (tgl_get_peer_type (chat_id) == TGL_PEER_CHAT);
  _tgl_do_send_photo (TLS, chat_id, file_name, chat_id, 0, 0, 0, 0, 0, NULL, 0, TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO, (void *)callback, callback_extra);
}

void tgl_do_set_channel_photo (struct tgl_state *TLS, tgl_peer_id_t chat_id, const char *file_name, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success), void *callback_extra) {
  assert (tgl_get_peer_type (chat_id) == TGL_PEER_CHANNEL);
  _tgl_do_send_photo (TLS, chat_id, file_name, chat_id, 0, 0, 0, 0, 0, NULL, 0, TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO, (void *)callback, callback_extra);
}

void tgl_do_set_profile_photo (struct tgl_state *TLS, const char *file_name, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  _tgl_do_send_photo (TLS, TLS->our_id, file_name, TLS->our_id, 0, 0, 0, 0, 0, NULL, 0, TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO, (void *)callback, callback_extra);
}
/* }}} */

/* {{{ Profile name */

int set_profile_name_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_user *DS_U = D;
  struct tgl_user *U = tglf_fetch_alloc_user (TLS, DS_U);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_user  *))q->callback) (TLS, q->callback_extra, 1, U);
  }
  return 0;
}

static struct query_methods set_profile_name_methods = {
  .on_answer = set_profile_name_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(user),
  .name = "set profile name"
};

void tgl_do_set_profile_name (struct tgl_state *TLS, const char *first_name, int first_name_len, const char *last_name, int last_name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra) {
  clear_packet ();
  out_int (CODE_account_update_profile);
  out_cstring (first_name, first_name_len);
  out_cstring (last_name, last_name_len);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_profile_name_methods, 0, callback, callback_extra);
}

void tgl_do_set_username (struct tgl_state *TLS, const char *username, int username_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra) {
  clear_packet ();
  out_int (CODE_account_update_username);
  out_cstring (username, username_len);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_profile_name_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Contacts search */

int contact_search_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_contacts_resolved_peer *DS_CRU = D;

  tgl_peer_id_t peer_id = tglf_fetch_peer_id (TLS, DS_CRU->peer);

  int i;
  for (i = 0; i < DS_LVAL (DS_CRU->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_CRU->users->data[i]);
  }
  
  for (i = 0; i < DS_LVAL (DS_CRU->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_CRU->chats->data[i]);
  }

  tgl_peer_t *P = tgl_peer_get (TLS, peer_id);

  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, tgl_peer_t *))q->callback) (TLS, q->callback_extra, 1, P);
  }

  return 0;
}

static struct query_methods contact_search_methods = {
  .on_answer = contact_search_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM(contacts_resolved_peer),
  .name = "contacts search"
};

void tgl_do_contact_search (struct tgl_state *TLS, const char *name, int name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, tgl_peer_t *U), void *callback_extra) {
  clear_packet ();
  out_int (CODE_contacts_resolve_username);
  out_cstring (name, name_len);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &contact_search_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Forward */

static int send_msgs_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct messages_send_extra *E = q->extra;

  tglu_work_any_updates (TLS, 1, D, (!E || E->multi) ? NULL : tgl_message_get (TLS, &E->id));
  tglu_work_any_updates (TLS, 0, D, (!E || E->multi) ? NULL : tgl_message_get (TLS, &E->id));


  if (!E) {
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
    }
  } else if (E->multi) {
    struct tgl_message **ML = talloc (sizeof (void *) * E->count);
    int count = E->count;
    int i;
    for (i = 0; i < count; i++) {
      struct tgl_message_id id;
      id.peer_type = TGL_PEER_RANDOM_ID;
      id.id = E->list[i].id;
      ML[i] = tgl_message_get (TLS, &id);
    }
    tfree (E->list, sizeof (tgl_message_id_t) * count);
    tfree (E, sizeof (*E));
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 1, count, ML);
    }
    tfree (ML, sizeof (void *) * count);
  } else {
    struct tgl_message_id id;
    id.peer_type = TGL_PEER_RANDOM_ID;
    id.id = E->id.id;
    struct tgl_message *M = tgl_message_get (TLS, &id);
    tfree (E, sizeof (*E));
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 1, M);
    }
  }
  return 0;
}

static int send_msgs_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  struct messages_send_extra *E = q->extra;

  if (!E) {
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 0);
    }
  } else if (E->multi) {
    int count = E->count;
    tfree (E->list, sizeof (tgl_message_id_t) * count);
    tfree (E, sizeof (*E));
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 0, 0, NULL);
    }
  } else {
    tfree (E, sizeof (*E));
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 0, NULL);
    }
  }
  return 0;
}

static struct query_methods send_msgs_methods = {
  .on_answer = send_msgs_on_answer,
  .on_error = send_msgs_on_error,
  .type = TYPE_TO_PARAM(updates),
  .name = "forward messages"
};

void tgl_do_forward_messages (struct tgl_state *TLS, tgl_peer_id_t id, int n, const tgl_message_id_t *_ids[], unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int count, struct tgl_message *ML[]), void *callback_extra) {
  if (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not forward messages to secret chats");
    if (callback) {
      callback (TLS, callback_extra, 0, 0, 0);
    }
    return;
  }
  tgl_peer_id_t from_id = TGL_MK_USER (0);
  tgl_message_id_t *ids = talloc (sizeof (tgl_message_id_t) * n);
  int i;
  for (i = 0; i < n; i++) {
    tgl_message_id_t msg_id = *_ids[i];
    if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
      msg_id = tgl_convert_temp_msg_id (TLS, msg_id);
    }
    if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
      tgl_set_query_error (TLS, EINVAL, "unknown message");
      if (callback) {
        callback (TLS, callback_extra, 0, 0, NULL);
      }
      tfree (ids, n * sizeof (tgl_message_id_t));
      return;
    }

    if (msg_id.peer_type == TGL_PEER_ENCR_CHAT) {
      tgl_set_query_error (TLS, EINVAL, "can not forward message from secret chat");
      if (callback) {
        callback (TLS, callback_extra, 0, 0, NULL);
      }
      tfree (ids, n * sizeof (tgl_message_id_t));
      return;
    }

    ids[i] = msg_id;

    if (i == 0) {      
      from_id = tgl_msg_id_to_peer_id (msg_id);
    } else {
      if (tgl_cmp_peer_id (from_id, tgl_msg_id_to_peer_id (msg_id))) {
        tgl_set_query_error (TLS, EINVAL, "can not forward messages from different dialogs");
        if (callback) {
          callback (TLS, callback_extra, 0, 0, NULL);
        }
        tfree (ids, n * sizeof (tgl_message_id_t));
        return;
      }
    }
  }
  
  
  clear_packet ();
  out_int (CODE_messages_forward_messages);

  unsigned f = 0;
  if (flags & TGLMF_POST_AS_CHANNEL) {
    f |= 16;
  }
  out_int (f);

  out_peer_id (TLS, from_id);

  out_int (CODE_vector);
  out_int (n);
  for (i = 0; i < n; i++) {
    out_int (ids[i].id);
  }

  struct messages_send_extra *E = talloc0 (sizeof (*E));
  E->multi = 1;
  E->count = n;
  E->list = talloc (sizeof (tgl_message_id_t) * n);
  out_int (CODE_vector);
  out_int (n);
  for (i = 0; i < n; i++) {
    E->list[i] = tgl_peer_id_to_random_msg_id (id);
    assert (E->list[i].id);
    out_long (E->list[i].id);
  }

  out_peer_id (TLS, id);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
        
  tfree (ids, n * sizeof (tgl_message_id_t));
}

void tgl_do_forward_message (struct tgl_state *TLS, tgl_peer_id_t peer_id, tgl_message_id_t *_msg_id, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t msg_id = *_msg_id;
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    msg_id = tgl_convert_temp_msg_id (TLS, msg_id);
  }
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (msg_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not forward messages from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (peer_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not forward messages to secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  
  clear_packet ();
  out_int (CODE_messages_forward_message);
  tgl_peer_id_t from_peer = tgl_msg_id_to_peer_id (msg_id);
  out_peer_id (TLS, from_peer);
  out_int (msg_id.id);

  struct messages_send_extra *E = talloc0 (sizeof (*E));
  E->id = tgl_peer_id_to_random_msg_id (peer_id);
  out_long (E->id.id);

  out_peer_id (TLS, peer_id);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
}

void tgl_do_send_contact (struct tgl_state *TLS, tgl_peer_id_t id, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not send contact to secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  int reply_id = flags >> 32;

  clear_packet ();
  out_int (CODE_messages_send_media);
  out_int (reply_id ? 1 : 0);
  if (reply_id) { out_int (reply_id); }
  out_peer_id (TLS, id);
  out_int (CODE_input_media_contact);
  out_cstring (phone, phone_len);
  out_cstring (first_name, first_name_len);
  out_cstring (last_name, last_name_len);

  struct messages_send_extra *E = talloc0 (sizeof (*E));
  tglt_secure_random (&E->id, 8);
  out_long (E->id.id);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
}


void tgl_do_reply_contact (struct tgl_state *TLS, tgl_message_id_t *_reply_id, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t reply_id = *_reply_id;
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    reply_id = tgl_convert_temp_msg_id (TLS, reply_id);
  }
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (reply_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not reply on message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  tgl_peer_id_t peer_id = tgl_msg_id_to_peer_id (reply_id);

  tgl_do_send_contact (TLS, peer_id, phone, phone_len, first_name, first_name_len, last_name, last_name_len, flags | TGL_SEND_MSG_FLAG_REPLY (reply_id.id), callback, callback_extra);
}

void tgl_do_forward_media (struct tgl_state *TLS, tgl_peer_id_t peer_id, tgl_message_id_t *_msg_id, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (tgl_get_peer_type (peer_id) == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not forward messages to secret chats");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  tgl_message_id_t msg_id = *_msg_id;
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    msg_id = tgl_convert_temp_msg_id (TLS, msg_id);
  }
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (msg_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not forward message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  struct tgl_message *M = tgl_message_get (TLS, &msg_id);
  if (!M || !(M->flags & TGLMF_CREATED) || (M->flags & TGLMF_ENCRYPTED)) {
    if (!M || !(M->flags & TGLMF_CREATED)) {
      tgl_set_query_error (TLS, EINVAL, "unknown message");
    } else {
      tgl_set_query_error (TLS, EINVAL, "can not forward message from secret chat");
    }
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (M->media.type != tgl_message_media_photo && M->media.type != tgl_message_media_document && M->media.type != tgl_message_media_audio && M->media.type != tgl_message_media_video) {
    tgl_set_query_error (TLS, EINVAL, "can only forward photo/document");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  clear_packet ();
  out_int (CODE_messages_send_media);
  int f = 0;
  if (flags & TGLMF_POST_AS_CHANNEL) {
    f |= 16;
  }
  out_int (f);
  out_peer_id (TLS, peer_id);
  switch (M->media.type) {
  case tgl_message_media_photo:
    assert (M->media.photo);
    out_int (CODE_input_media_photo);
    out_int (CODE_input_photo);
    out_long (M->media.photo->id);
    out_long (M->media.photo->access_hash);
    out_string ("");
    break;
  case tgl_message_media_document:
  case tgl_message_media_audio:
  case tgl_message_media_video:
    assert (M->media.document);
    out_int (CODE_input_media_document);
    out_int (CODE_input_document);
    out_long (M->media.document->id);
    out_long (M->media.document->access_hash);
    out_string ("");
    break;
  default:
    assert (0);
  }

  struct messages_send_extra *E = talloc0 (sizeof (*E));
  E->id = tgl_peer_id_to_random_msg_id (peer_id);
  out_long (E->id.id);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
}
/* }}} */

/* {{{ Send location */

void tgl_do_send_location (struct tgl_state *TLS, tgl_peer_id_t peer_id, double latitude, double longitude, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (tgl_get_peer_type (peer_id) == TGL_PEER_ENCR_CHAT) {
    tgl_do_send_location_encr (TLS, peer_id, latitude, longitude, flags, callback, callback_extra);
  } else {
    int reply_id = flags >> 32;
    clear_packet ();
    out_int (CODE_messages_send_media);
    unsigned f = reply_id ? 1 : 0;
    if (flags & TGLMF_POST_AS_CHANNEL) {
      f |= 16;
    }
    out_int (f);
    if (reply_id) { out_int (reply_id); }
    out_peer_id (TLS, peer_id);
    out_int (CODE_input_media_geo_point);
    out_int (CODE_input_geo_point);
    out_double (latitude);
    out_double (longitude);

    struct messages_send_extra *E = talloc0 (sizeof (*E));
    E->id = tgl_peer_id_to_random_msg_id (peer_id);
    out_long (E->id.id);

    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
  }
}

void tgl_do_reply_location (struct tgl_state *TLS, tgl_message_id_t *_reply_id, double latitude, double longitude, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t reply_id = *_reply_id;
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    reply_id = tgl_convert_temp_msg_id (TLS, reply_id);
  }
  if (reply_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  if (reply_id.peer_type == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not reply on message from secret chat");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  tgl_peer_id_t peer_id = tgl_msg_id_to_peer_id (reply_id);

  tgl_do_send_location (TLS, peer_id, latitude, longitude, flags | TGL_SEND_MSG_FLAG_REPLY (reply_id.id), callback, callback_extra);
}
/* }}} */

/* {{{ Rename chat */

void tgl_do_rename_chat (struct tgl_state *TLS, tgl_peer_id_t id, const char *name, int name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_edit_chat_title);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHAT);
  out_int (tgl_get_peer_id (id));
  out_cstring (name, name_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

 /* {{{ Rename channel */

void tgl_do_rename_channel (struct tgl_state *TLS, tgl_peer_id_t id, const char *name, int name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_edit_title);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  out_cstring (name, name_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

 /* {{{ Join channel */

void tgl_do_join_channel (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_join_channel);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Leave channel */

void tgl_do_leave_channel (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_leave_channel);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ channel change about */

static int channels_set_about_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods channels_set_about_methods = {
  .on_answer = channels_set_about_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "channels set about"
};

void tgl_do_channel_set_about (struct tgl_state *TLS, tgl_peer_id_t id, const char *about, int about_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_edit_about);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  out_cstring (about, about_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &channels_set_about_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Channel set username */
void tgl_do_channel_set_username (struct tgl_state *TLS, tgl_peer_id_t id, const char *username, int username_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_update_username);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  out_cstring (username, username_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &channels_set_about_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Channel set admin */
void tgl_do_channel_set_admin (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t user_id, int type, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_edit_admin);
  assert (tgl_get_peer_type (channel_id) == TGL_PEER_CHANNEL);
  assert (tgl_get_peer_type (user_id) == TGL_PEER_USER);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (channel_id));
  out_long (channel_id.access_hash);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (user_id));
  out_long (user_id.access_hash);
  switch (type) {
  case 1:
    out_int (CODE_channel_role_moderator);
    break;
  case 2:
    out_int (CODE_channel_role_editor);
    break;
  default:
    out_int (CODE_channel_role_empty);
    break;
  }
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Channel members */
struct channel_get_members_extra {
  int size;
  int count;
  struct tgl_user **UL;
  int type;
  int offset;
  int limit;
  tgl_peer_id_t id;
};

void _tgl_do_channel_get_members  (struct tgl_state *TLS, struct channel_get_members_extra *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *UL[]), void *callback_extra);

static int channels_get_members_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_channels_channel_participants *DS_CP = D;
  
  int count = DS_LVAL (DS_CP->participants->cnt);

  struct channel_get_members_extra *E = q->extra;


  if (E->count + count > E->size) {
    E->UL = trealloc (E->UL, E->size * sizeof (void *), (E->count + count) * sizeof (void *));
    E->size = E->count + count;
  }
  int i;
  for (i = 0; i < DS_LVAL (DS_CP->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_CP->users->data[i]);
  }
  for (i = 0; i < count; i++) {
    E->UL[E->count ++] = (void *)tgl_peer_get (TLS, TGL_MK_USER (DS_LVAL (DS_CP->participants->data[i]->user_id)));
  }
  E->offset += count;
  
  if (!count || E->count == E->limit) {
    ((void (*)(struct tgl_state *, void *, int, int, struct tgl_user **))q->callback)(TLS, q->callback_extra, 1, E->count, E->UL);
    tfree (E->UL, E->size * sizeof (void *));
    tfree (E, sizeof (*E));    
    return 0;
  }
  _tgl_do_channel_get_members (TLS, E, q->callback, q->callback_extra);
  return 0;
}

static int channels_get_members_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int, int, void *))(q->callback))(TLS, q->callback_extra, 0, 0, NULL);
  }
  
  struct channel_get_members_extra *E = q->extra;
  tfree (E->UL, E->size * sizeof (void *));
  tfree (E, sizeof (*E));    

  return 0;
}

static struct query_methods channels_get_members_methods = {
  .on_answer = channels_get_members_on_answer,
  .on_error = channels_get_members_on_error,
  .type = TYPE_TO_PARAM(channels_channel_participants),
  .name = "channels get members"
};

void _tgl_do_channel_get_members  (struct tgl_state *TLS, struct channel_get_members_extra *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *UL[]), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_get_participants);
  assert (tgl_get_peer_type (E->id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (E->id.peer_id);
  out_long (E->id.access_hash);

  switch (E->type) {
  case 1:
  case 2:
    out_int (CODE_channel_participants_admins);
    break;
  case 3:
    out_int (CODE_channel_participants_kicked);
    break;
  case 4:
    out_int (CODE_channel_participants_bots);
    break;
  default:
    out_int (CODE_channel_participants_recent);
    break;
  }
  out_int (E->offset);
  out_int (E->limit);
  
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &channels_get_members_methods, E, callback, callback_extra);
}

void tgl_do_channel_get_members  (struct tgl_state *TLS, tgl_peer_id_t channel_id, int limit, int offset, int type, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *UL[]), void *callback_extra) {
  struct channel_get_members_extra *E = talloc0 (sizeof (*E));
  E->type = type;
  E->id = channel_id;
  E->limit = limit;
  E->offset = offset;
  _tgl_do_channel_get_members (TLS, E, callback, callback_extra);
}
/* }}} */

/* {{{ Chat info */

static int chat_info_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_chat *C = tglf_fetch_alloc_chat_full (TLS, D);
  //print_chat_info (C);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_chat *))q->callback) (TLS, q->callback_extra, 1, C);
  }
  return 0;
}

static struct query_methods chat_info_methods = {
  .on_answer = chat_info_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(messages_chat_full),
  .name = "chat info"
};

void tgl_do_get_chat_info (struct tgl_state *TLS, tgl_peer_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_chat *C), void *callback_extra) {
  if (offline_mode) {
    tgl_peer_t *C = tgl_peer_get (TLS, id);
    if (!C) {
      tgl_set_query_error (TLS, EINVAL, "unknown chat id");
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        callback (TLS, callback_extra, 1, &C->chat);
      }
    }
    return;
  }
  clear_packet ();
  out_int (CODE_messages_get_full_chat);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHAT);
  out_int (tgl_get_peer_id (id));
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &chat_info_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Channel info */

static int channel_info_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_channel *C = tglf_fetch_alloc_channel_full (TLS, D);
  //print_chat_info (C);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_channel *))q->callback) (TLS, q->callback_extra, 1, C);
  }
  return 0;
}

static struct query_methods channel_info_methods = {
  .on_answer = channel_info_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(messages_chat_full),
  .name = "channel info"
};

void tgl_do_get_channel_info (struct tgl_state *TLS, tgl_peer_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_channel *C), void *callback_extra) {
  if (offline_mode) {
    tgl_peer_t *C = tgl_peer_get (TLS, id);
    if (!C) {
      tgl_set_query_error (TLS, EINVAL, "unknown chat id");
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        callback (TLS, callback_extra, 1, &C->channel);
      }
    }
    return;
  }
  clear_packet ();
  out_int (CODE_channels_get_full_channel);
  assert (tgl_get_peer_type (id) == TGL_PEER_CHANNEL);
  out_int (CODE_input_channel);
  out_int (id.peer_id);
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &channel_info_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ User info */

static int user_info_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_user *U = tglf_fetch_alloc_user_full (TLS, D);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_user *))q->callback) (TLS, q->callback_extra, 1, U);
  }
  return 0;
}

static struct query_methods user_info_methods = {
  .on_answer = user_info_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(user_full),
  .name = "user info"
};

void tgl_do_get_user_info (struct tgl_state *TLS, tgl_peer_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_USER) {
    tgl_set_query_error (TLS, EINVAL, "id should be user id");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }
  if (offline_mode) {
    tgl_peer_t *C = tgl_peer_get (TLS, id);
    if (!C) {
      tgl_set_query_error (TLS, EINVAL, "unknown user id");
      if (callback) {
        callback (TLS, callback_extra, 0, 0);
      }
    } else {
      if (callback) {
        callback (TLS, callback_extra, 1, &C->user);
      }
    }
    return;
  }
  clear_packet ();
  out_int (CODE_users_get_full_user);
  assert (tgl_get_peer_type (id) == TGL_PEER_USER);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &user_info_methods, 0, callback, callback_extra);
}

static void resend_query_cb (struct tgl_state *TLS, void *_q, int success) {
  assert (success);

  bl_do_dc_signed (TLS, TLS->DC_working->id);

  struct query *q = _q;

  clear_packet ();
  out_int (CODE_users_get_full_user);
  out_int (CODE_input_user_self);
  tglq_send_query (TLS, q->DC, packet_ptr - packet_buffer, packet_buffer, &user_info_methods, 0, q->callback, q->callback_extra);

  tfree (q->data, 4 * q->data_len);
  TLS->timer_methods->free (q->ev);
  tfree (q, sizeof (*q));
}
/* }}} */

/* {{{ Load photo/video */
struct download {
  int offset;
  int size;
  long long volume;
  long long secret;
  long long access_hash;
  int local_id;
  int dc;
  int next;
  int fd;
  char *name;
  char *ext;
  long long id;
  unsigned char *iv;
  unsigned char *key;
  int type;
  int refcnt;
};


static void end_load (struct tgl_state *TLS, struct download *D, void *callback, void *callback_extra) {
  TLS->cur_downloading_bytes -= D->size;
  TLS->cur_downloaded_bytes -= D->size;

  if (D->fd >= 0) {
    close (D->fd);
  }

  if (callback) {
    ((void (*)(struct tgl_state *, void *, int, char *))callback) (TLS, callback_extra, 1, D->name);
  }

  if (D->iv) {
    tfree_secure (D->iv, 32);
  }
  tfree_str (D->name);
  tfree (D, sizeof (*D));
}

static void load_next_part (struct tgl_state *TLS, struct download *D, void *callback, void *callback_extra);
static int download_on_answer (struct tgl_state *TLS, struct query *q, void *DD) {
  struct tl_ds_upload_file *DS_UF = DD;

  struct download *D = q->extra;
  if (D->fd == -1) {
    D->fd = open (D->name, O_CREAT | O_WRONLY | O_BINARY, 0640);
    if (D->fd < 0) {
      tgl_set_query_error (TLS, EBADF, "Can not open file for writing: %s", strerror(errno));
      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int, char *))q->callback) (TLS, q->callback_extra, 0, NULL);
      }

      if (D->iv) {
        tfree_secure (D->iv, 32);
      }
      tfree_str (D->name);
      if (D->ext) {
        tfree_str (D->ext);
      }
      tfree (D, sizeof (*D));
      return 0;
    }
  }

  int len = DS_UF->bytes->len;
  TLS->cur_downloaded_bytes += len;
  //update_prompt ();

  if (D->iv) {
    assert (!(len & 15));
    void *ptr = DS_UF->bytes->data;

    TGLC_aes_key aes_key;
    TGLC_aes_set_decrypt_key (D->key, 256, &aes_key);
    TGLC_aes_ige_encrypt (ptr, ptr, len, &aes_key, D->iv, 0);
    memset (&aes_key, 0, sizeof (aes_key));
    if (len > D->size - D->offset) {
      len = D->size - D->offset;
    }
    assert (write (D->fd, ptr, len) == len);
  } else {
    assert (write (D->fd, DS_UF->bytes->data, len) == len);
  }

  D->offset += len;
  D->refcnt --;
  if (D->offset < D->size) {
    load_next_part (TLS, D, q->callback, q->callback_extra);
    return 0;
  } else {
    if (!D->refcnt) {
      end_load (TLS, D, q->callback, q->callback_extra);
    }
    return 0;
  }
}

static int download_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct download *D = q->extra;
  if (D->fd >= 0) {
    close (D->fd);
  }

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, char *))q->callback) (TLS, q->callback_extra, 0, NULL);
  }

  if (D->iv) {
    tfree_secure (D->iv, 32);
  }
  tfree_str (D->name);
  if (D->ext) {
    tfree_str (D->ext);
  }
  tfree (D, sizeof (*D));
  return 0;
}

static struct query_methods download_methods = {
  .on_answer = download_on_answer,
  .on_error = download_on_error,
  .type = TYPE_TO_PARAM(upload_file),
  .name = "download part"
};

static void load_next_part (struct tgl_state *TLS, struct download *D, void *callback, void *callback_extra) {
  if (!D->offset) {
    static char buf[PATH_MAX];
    int l;
    if (!D->id) {
      l = tsnprintf (buf, sizeof (buf), "%s/download_%" INT64_PRINTF_MODIFIER "d_%d.jpg", TLS->downloads_directory, D->volume, D->local_id);
    } else {
      if (D->ext) {
        l = tsnprintf (buf, sizeof (buf), "%s/download_%" INT64_PRINTF_MODIFIER "d.%s", TLS->downloads_directory, D->id, D->ext);
      } else {
        l = tsnprintf (buf, sizeof (buf), "%s/download_%" INT64_PRINTF_MODIFIER "d", TLS->downloads_directory, D->id);
      }
    }
    if (l >= (int) sizeof (buf)) {
      vlogprintf (E_ERROR, "Download filename is too long");
      assert (0);
    }
    D->name = tstrdup (buf);
    struct stat st;
    if (stat (buf, &st) >= 0) {
      D->offset = st.st_size;
      if (D->offset >= D->size) {
        TLS->cur_downloading_bytes += D->size;
        TLS->cur_downloaded_bytes += D->offset;
        vlogprintf (E_NOTICE, "Already downloaded\n");
        end_load (TLS, D, callback, callback_extra);
        return;
      }
    }

    TLS->cur_downloading_bytes += D->size;
    TLS->cur_downloaded_bytes += D->offset;
    //update_prompt ();
  }
  D->refcnt ++;
  clear_packet ();
  out_int (CODE_upload_get_file);
  if (!D->id) {
    out_int (CODE_input_file_location);
    out_long (D->volume);
    out_int (D->local_id);
    out_long (D->secret);
  } else {
    if (D->iv) {
      out_int (CODE_input_encrypted_file_location);
    } else {
      out_int (D->type);
    }
    out_long (D->id);
    out_long (D->access_hash);
  }
  out_int (D->offset);
  out_int (D->size ? (1 << 14) : (1 << 19));
  tglq_send_query (TLS, TLS->DC_list[D->dc], packet_ptr - packet_buffer, packet_buffer, &download_methods, D, callback, callback_extra);
  //tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &download_methods, D);
}

void tgl_do_load_photo_size (struct tgl_state *TLS, struct tgl_photo_size *P, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  if (!P->loc.dc) {
    vlogprintf (E_WARNING, "Bad video thumb\n");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  assert (P);
  struct download *D = talloc0 (sizeof (*D));
  D->id = 0;
  D->offset = 0;
  D->size = P->size;
  D->volume = P->loc.volume;
  D->dc = P->loc.dc;
  D->local_id = P->loc.local_id;
  D->secret = P->loc.secret;
  D->name = 0;
  D->fd = -1;
  load_next_part (TLS, D, callback, callback_extra);
}

void tgl_do_load_file_location (struct tgl_state *TLS, struct tgl_file_location *P, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  if (!P->dc) {
    tgl_set_query_error (TLS, EINVAL, "Bad file location");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }

  assert (P);
  struct download *D = talloc0 (sizeof (*D));
  D->id = 0;
  D->offset = 0;
  D->size = 0;
  D->volume = P->volume;
  D->dc = P->dc;
  D->local_id = P->local_id;
  D->secret = P->secret;
  D->name = 0;
  D->fd = -1;
  load_next_part (TLS, D, callback, callback_extra);
}

void tgl_do_load_photo (struct tgl_state *TLS, struct tgl_photo *photo, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  if (!photo->sizes_num) {
    tgl_set_query_error (TLS, EINVAL, "Bad photo (no photo sizes");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
  int max = -1;
  int maxi = 0;
  int i;
  for (i = 0; i < photo->sizes_num; i++) {
    if (photo->sizes[i].w + photo->sizes[i].h > max) {
      max = photo->sizes[i].w + photo->sizes[i].h;
      maxi = i;
    }
  }
  tgl_do_load_photo_size (TLS, &photo->sizes[maxi], callback, callback_extra);
}

void tgl_do_load_document_thumb (struct tgl_state *TLS, struct tgl_document *video, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  tgl_do_load_photo_size (TLS, &video->thumb, callback, callback_extra);
}

static void _tgl_do_load_document (struct tgl_state *TLS, struct tgl_document *V, struct download *D, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  assert (V);
  D->offset = 0;
  D->size = V->size;
  D->id = V->id;
  D->access_hash = V->access_hash;
  D->dc = V->dc_id;
  D->name = 0;
  D->fd = -1;
  
  if (V->mime_type) {
    char *r = tg_extension_by_mime (V->mime_type);
    if (r) {
      D->ext = tstrdup (r);
    }
  }
  load_next_part (TLS, D, callback, callback_extra);
}

void tgl_do_load_document (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  
  struct download *D = talloc0 (sizeof (*D));
  D->type = CODE_input_document_file_location;
  
  _tgl_do_load_document (TLS, V, D, callback, callback_extra);
}

void tgl_do_load_video (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {

  struct download *D = talloc0 (sizeof (*D));
  D->type = CODE_input_video_file_location;
  
  _tgl_do_load_document (TLS, V, D, callback, callback_extra);
}

void tgl_do_load_audio (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  
  struct download *D = talloc0 (sizeof (*D));
  D->type = CODE_input_audio_file_location;

  _tgl_do_load_document (TLS, V, D, callback, callback_extra);
}

void tgl_do_load_encr_document (struct tgl_state *TLS, struct tgl_encr_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *filename), void *callback_extra) {
  assert (V);
  struct download *D = talloc0 (sizeof (*D));
  D->offset = 0;
  D->size = V->size;
  D->id = V->id;
  D->access_hash = V->access_hash;
  D->dc = V->dc_id;
  D->name = 0;
  D->fd = -1;
  D->key = V->key;
  D->iv = talloc (32);
  memcpy (D->iv, V->iv, 32);
  if (V->mime_type) {
    char *r = tg_extension_by_mime (V->mime_type);
    if (r) {
      D->ext = tstrdup (r);
    }
  }
  load_next_part (TLS, D, callback, callback_extra);

  unsigned char md5[16];
  unsigned char str[64];
  memcpy (str, V->key, 32);
  memcpy (str + 32, V->iv, 32);
  TGLC_md5 (str, 64, md5);
  assert (V->key_fingerprint == ((*(int *)md5) ^ (*(int *)(md5 + 4))));
}
/* }}} */

/* {{{ Export auth */

static int import_auth_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_auth_authorization *DS_U = D;
  tglf_fetch_alloc_user (TLS, DS_U->user);

  bl_do_dc_signed (TLS, ((struct tgl_dc *)q->extra)->id);

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods import_auth_methods = {
  .on_answer = import_auth_on_answer,
  .on_error = fail_on_error,
  .type = TYPE_TO_PARAM(auth_authorization),
  .name = "import authorization"
};

static int export_auth_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_auth_exported_authorization *DS_EA = D;

  bl_do_set_our_id (TLS, TGL_MK_USER (DS_LVAL (DS_EA->id)));


  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_auth_import_authorization);
  out_int (tgl_get_peer_id (TLS->our_id));
  out_cstring (DS_STR (DS_EA->bytes));
  tglq_send_query (TLS, q->extra, packet_ptr - packet_buffer, packet_buffer, &import_auth_methods, q->extra, q->callback, q->callback_extra);
  return 0;
}

static struct query_methods export_auth_methods = {
  .on_answer = export_auth_on_answer,
  .on_error = fail_on_error,
  .type = TYPE_TO_PARAM(auth_exported_authorization),
  .name = "export authorization"
};

void tgl_do_export_auth (struct tgl_state *TLS, int num, void (*callback) (struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_auth_export_authorization);
  out_int (num);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &export_auth_methods, TLS->DC_list[num], callback, callback_extra);
}
/* }}} */

/* {{{ Add contact */
static int add_contact_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_contacts_imported_contacts *DS_CIC = D;

  if (DS_LVAL (DS_CIC->imported->cnt) > 0) {
    vlogprintf (E_DEBUG, "Added successfully");
  } else {
    vlogprintf (E_DEBUG, "Not added");
  }

  int n = DS_LVAL (DS_CIC->users->cnt);

  struct tgl_user **UL = talloc (n * sizeof (void *));
  int i;
  for (i = 0; i < n; i++) {
    UL[i] = tglf_fetch_alloc_user (TLS, DS_CIC->users->data[i]);
  }

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, int, struct tgl_user **))q->callback) (TLS, q->callback_extra, 1, n, UL);
  }
  tfree (UL, n * sizeof (void *));
  return 0;
}

static struct query_methods add_contact_methods = {
  .on_answer = add_contact_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM(contacts_imported_contacts),
  .name = "add contact"
};

void tgl_do_add_contact (struct tgl_state *TLS, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, int force, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, int size, struct tgl_user *users[]), void *callback_extra) {
  clear_packet ();
  out_int (CODE_contacts_import_contacts);
  out_int (CODE_vector);
  out_int (1);
  out_int (CODE_input_phone_contact);
  long long r;
  tglt_secure_random (&r, 8);
  out_long (r);
  out_cstring (phone, phone_len);
  out_cstring (first_name, first_name_len);
  out_cstring (last_name, last_name_len);
  out_int (force ? CODE_bool_true : CODE_bool_false);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &add_contact_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Del contact */
static int del_contact_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods del_contact_methods = {
  .on_answer = del_contact_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(contacts_link),
  .name = "del contact"
};

void tgl_do_del_contact (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_USER) {
    tgl_set_query_error (TLS, EINVAL, "id should be user id");
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  clear_packet ();
  out_int (CODE_contacts_delete_contact);

  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &del_contact_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Msg search */

struct msg_search_extra {
  struct tgl_message **ML;
  int list_offset;
  int list_size;
  tgl_peer_id_t id;
  int limit;
  int offset;
  int from;
  int to;
  int max_id;
  char *query;
};

static void _tgl_do_msg_search (struct tgl_state *TLS, struct msg_search_extra *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra);

static int msg_search_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_messages *DS_MM = D;
  
  int i;
  for (i = 0; i < DS_LVAL (DS_MM->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_MM->chats->data[i]);
  }
  for (i = 0; i < DS_LVAL (DS_MM->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_MM->users->data[i]);
  }

  struct msg_search_extra *E = q->extra;

  int n = DS_LVAL (DS_MM->messages->cnt);

  if (E->list_size - E->list_offset < n) {
    int new_list_size = 2 * E->list_size;
    if (new_list_size - E->list_offset < n) {
      new_list_size = n + E->list_offset;
    }
    E->ML = trealloc (E->ML, E->list_size * sizeof (void *), new_list_size * sizeof (void *));
    assert (E->ML);
    E->list_size = new_list_size;
  }

  for (i = 0; i < n; i++) {
    E->ML[i + E->list_offset] = tglf_fetch_alloc_message (TLS, DS_MM->messages->data[i], NULL);
  }
  E->list_offset += n;
  E->offset += n;
  E->limit -= n;
  if (E->limit + E->offset >= DS_LVAL (DS_MM->count)) {
    E->limit = DS_LVAL (DS_MM->count) - E->offset;
    if (E->limit < 0) { E->limit = 0; }
  }
  assert (E->limit >= 0);

  if (E->limit <= 0 || DS_MM->magic == CODE_messages_messages) {
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 1, E->list_offset, E->ML);
    }

    tfree_str (E->query);
    tfree (E->ML, sizeof (void *) * E->list_size);
    tfree (E, sizeof (*E));
  } else {
    E->max_id = E->ML[E->list_offset - 1]->permanent_id.id;
    E->offset = 0;
    _tgl_do_msg_search (TLS, E, q->callback, q->callback_extra);
  }
  return 0;
}

static int msg_search_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);

  struct msg_search_extra *E = q->extra;
  tfree_str (E->query);
  tfree (E->ML, sizeof (void *) * E->list_size);
  tfree (E, sizeof (*E));
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, int, struct tgl_message **))q->callback) (TLS, q->callback_extra, 0, 0, NULL);
  }
  return 0;
}

static struct query_methods msg_search_methods = {
  .on_answer = msg_search_on_answer,
  .on_error = msg_search_on_error,
  .type = TYPE_TO_PARAM(messages_messages),
  .name = "messages search"
};

static void _tgl_do_msg_search (struct tgl_state *TLS, struct msg_search_extra *E, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra) {
  clear_packet ();
  if (tgl_get_peer_type (E->id) == TGL_PEER_UNKNOWN) {
    out_int (CODE_messages_search_global);
    out_string (E->query);
    out_int (0);
    out_int (CODE_input_peer_empty);
    out_int (E->offset);
    out_int (E->limit);
  } else {
    out_int (CODE_messages_search);
    out_int (0);
    out_peer_id (TLS, E->id);

    out_string (E->query);
    out_int (CODE_input_messages_filter_empty);
    out_int (E->from);
    out_int (E->to);
    out_int (E->offset); // offset
    out_int (E->max_id); // max_id
    out_int (E->limit);
  }
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &msg_search_methods, E, callback, callback_extra);
}

void tgl_do_msg_search (struct tgl_state *TLS, tgl_peer_id_t id, int from, int to, int limit, int offset, const char *pattern, int pattern_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra) {
  if (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "can not search in secret chats");
    if (callback) {
      callback (TLS, callback_extra, 0, 0, 0);
    }
    return;
  }
  struct msg_search_extra *E = talloc0 (sizeof (*E));
  E->id = id;
  E->from = from;
  E->to = to;
  E->limit = limit;
  E->offset = offset;
  E->query = tstrndup (pattern, pattern_len);

  _tgl_do_msg_search (TLS, E, callback, callback_extra);
}
/* }}} */

/* {{{ Get difference */

static int get_state_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_updates_state *DS_US = D;

  assert (TLS->locks & TGL_LOCK_DIFF);
  TLS->locks ^= TGL_LOCK_DIFF;

  bl_do_set_pts (TLS, DS_LVAL (DS_US->pts));
  bl_do_set_qts (TLS, DS_LVAL (DS_US->qts));
  bl_do_set_date (TLS, DS_LVAL (DS_US->date));
  bl_do_set_seq (TLS, DS_LVAL (DS_US->seq));

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
  }
  return 0;
}

static int lookup_state_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_updates_state *DS_US = D;
  int pts = DS_LVAL (DS_US->pts);
  int qts = DS_LVAL (DS_US->qts);
  int seq = DS_LVAL (DS_US->seq);

  if (pts > TLS->pts || qts > TLS->qts || seq > TLS->seq) {
    tgl_do_get_difference (TLS, 0, 0, 0);
  }
  return 0;
}


//int get_difference_active;
static int get_difference_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_updates_difference *DS_UD = D;

  assert (TLS->locks & TGL_LOCK_DIFF);
  TLS->locks ^= TGL_LOCK_DIFF;

  if (DS_UD->magic == CODE_updates_difference_empty) {
    bl_do_set_date (TLS, DS_LVAL (DS_UD->date));
    bl_do_set_seq (TLS, DS_LVAL (DS_UD->seq));

    vlogprintf (E_DEBUG, "Empty difference. Seq = %d\n", TLS->seq);
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
    }
  } else {
    int i;

    for (i = 0; i < DS_LVAL (DS_UD->users->cnt); i++) {
      tglf_fetch_alloc_user (TLS, DS_UD->users->data[i]);
    }
    for (i = 0; i < DS_LVAL (DS_UD->chats->cnt); i++) {
      tglf_fetch_alloc_chat (TLS, DS_UD->chats->data[i]);
    }

    int ml_pos = DS_LVAL (DS_UD->new_messages->cnt);
    struct tgl_message **ML = talloc (ml_pos * sizeof (void *));
    for (i = 0; i < ml_pos; i++) {
      ML[i] = tglf_fetch_alloc_message (TLS, DS_UD->new_messages->data[i], NULL);
    }

    int el_pos = DS_LVAL (DS_UD->new_encrypted_messages->cnt);
    struct tgl_message **EL = talloc (el_pos * sizeof (void *));
    for (i = 0; i < el_pos; i++) {
      EL[i] = tglf_fetch_alloc_encrypted_message (TLS, DS_UD->new_encrypted_messages->data[i]);
    }

    for (i = 0; i < DS_LVAL (DS_UD->other_updates->cnt); i++) {
      tglu_work_update (TLS, 1, DS_UD->other_updates->data[i]);
    }

    for (i = 0; i < DS_LVAL (DS_UD->other_updates->cnt); i++) {
      tglu_work_update (TLS, -1, DS_UD->other_updates->data[i]);
    }

    for (i = 0; i < ml_pos; i++) {
      bl_do_msg_update (TLS, &ML[i]->permanent_id);
    }
    for (i = 0; i < el_pos; i++) {
      // messages to secret chats that no longer exist are not initialized and NULL
      if (EL[i]) {
        bl_do_msg_update (TLS, &EL[i]->permanent_id);
      }
    }

    tfree (ML, ml_pos * sizeof (void *));
    tfree (EL, el_pos * sizeof (void *));

    if (DS_UD->state) {
      bl_do_set_pts (TLS, DS_LVAL (DS_UD->state->pts));
      bl_do_set_qts (TLS, DS_LVAL (DS_UD->state->qts));
      bl_do_set_date (TLS, DS_LVAL (DS_UD->state->date));
      bl_do_set_seq (TLS, DS_LVAL (DS_UD->state->seq));

      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
      }
    } else {
      bl_do_set_pts (TLS, DS_LVAL (DS_UD->intermediate_state->pts));
      bl_do_set_qts (TLS, DS_LVAL (DS_UD->intermediate_state->qts));
      bl_do_set_date (TLS, DS_LVAL (DS_UD->intermediate_state->date));

      tgl_do_get_difference (TLS, 0, q->callback, q->callback_extra);
    }
  }
  return 0;
}

static struct query_methods lookup_state_methods = {
  .on_answer = lookup_state_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(updates_state),
  .name = "lookup state"
};

static struct query_methods get_state_methods = {
  .on_answer = get_state_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(updates_state),
  .name = "get state"
};

static struct query_methods get_difference_methods = {
  .on_answer = get_difference_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(updates_difference),
  .name = "get difference"
};

void tgl_do_lookup_state (struct tgl_state *TLS) {
  if (TLS->locks & TGL_LOCK_DIFF) {
    return;
  }
  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_updates_get_state);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &lookup_state_methods, 0, 0, 0);
}

void tgl_do_get_difference (struct tgl_state *TLS, int sync_from_start, void (*callback)(struct tgl_state *tls, void *callback_extra, int success), void *callback_extra) {
  //get_difference_active = 1;
  //difference_got = 0;
  if (TLS->locks & TGL_LOCK_DIFF) {
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  TLS->locks |= TGL_LOCK_DIFF;
  clear_packet ();
  tgl_do_insert_header (TLS);
  if (TLS->pts > 0 || sync_from_start) {
    if (TLS->pts == 0) { TLS->pts = 1; }
    //if (TLS->qts == 0) { TLS->qts = 1; }
    if (TLS->date == 0) { TLS->date = 1; }
    out_int (CODE_updates_get_difference);
    out_int (TLS->pts);
    out_int (TLS->date);
    out_int (TLS->qts);
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_difference_methods, 0, callback, callback_extra);
  } else {
    out_int (CODE_updates_get_state);
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_state_methods, 0, callback, callback_extra);
  }
}
/* }}} */

/* {{{ Get channel difference */
void tgl_do_get_channel_difference (struct tgl_state *TLS, int id, void (*callback)(struct tgl_state *tls, void *callback_extra, int success), void *callback_extra);

static int get_channel_difference_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_updates_channel_difference *DS_UD = D;

  tgl_peer_t *E = q->extra;

  assert (E->flags & TGLCHF_DIFF);
  E->flags ^= TGLCHF_DIFF;

  if (DS_UD->magic == CODE_updates_channel_difference_empty) {
    bl_do_set_channel_pts (TLS, tgl_get_peer_id (E->id), DS_LVAL (DS_UD->channel_pts));

    vlogprintf (E_DEBUG, "Empty difference. Seq = %d\n", TLS->seq);
    if (q->callback) {
      ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
    }
  } else {
    int i;

    for (i = 0; i < DS_LVAL (DS_UD->users->cnt); i++) {
      tglf_fetch_alloc_user (TLS, DS_UD->users->data[i]);
    }
    for (i = 0; i < DS_LVAL (DS_UD->chats->cnt); i++) {
      tglf_fetch_alloc_chat (TLS, DS_UD->chats->data[i]);
    }

    int ml_pos = DS_LVAL (DS_UD->new_messages->cnt);
    struct tgl_message **ML = talloc (ml_pos * sizeof (void *));
    for (i = 0; i < ml_pos; i++) {
      ML[i] = tglf_fetch_alloc_message (TLS, DS_UD->new_messages->data[i], NULL);
    }

    for (i = 0; i < DS_LVAL (DS_UD->other_updates->cnt); i++) {
      tglu_work_update (TLS, 1, DS_UD->other_updates->data[i]);
    }

    for (i = 0; i < DS_LVAL (DS_UD->other_updates->cnt); i++) {
      tglu_work_update (TLS, -1, DS_UD->other_updates->data[i]);
    }

    for (i = 0; i < ml_pos; i++) {
      bl_do_msg_update (TLS, &ML[i]->permanent_id);
    }

    tfree (ML, ml_pos * sizeof (void *));

    bl_do_set_channel_pts (TLS, tgl_get_peer_id (E->id), DS_LVAL (DS_UD->channel_pts));
    if (DS_UD->magic != CODE_updates_channel_difference_too_long) {
      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
      }
    } else {
      tgl_do_get_channel_difference (TLS, tgl_get_peer_id (E->id), q->callback, q->callback_extra);
    }
  }
  return 0;
}

static struct query_methods get_channel_difference_methods = {
  .on_answer = get_channel_difference_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(updates_channel_difference),
  .name = "get channel difference"
};

void tgl_do_get_channel_difference (struct tgl_state *TLS, int id, void (*callback)(struct tgl_state *tls, void *callback_extra, int success), void *callback_extra) {
  tgl_peer_t *E = tgl_peer_get (TLS, TGL_MK_CHANNEL (id));

  if (!E || !(E->flags & TGLPF_CREATED) || !E->channel.pts) { 
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  //get_difference_active = 1;
  //difference_got = 0;
  if (E->flags & TGLCHF_DIFF) {
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  E->flags |= TGLCHF_DIFF;
  clear_packet ();
  tgl_do_insert_header (TLS);

  out_int (CODE_updates_get_channel_difference);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (E->id));
  out_long (E->channel.access_hash);

  out_int (CODE_channel_messages_filter_empty);
  out_int (E->channel.pts);
  out_int (100);
  
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_channel_difference_methods, E, callback, callback_extra);
}
/* }}} */

/* {{{ Visualize key */

int tgl_do_visualize_key (struct tgl_state *TLS, tgl_peer_id_t id, unsigned char buf[16]) {
  assert (tgl_get_peer_type (id) == TGL_PEER_ENCR_CHAT);
  tgl_peer_t *P = tgl_peer_get (TLS, id);
  assert (P);
  if (P->encr_chat.state != sc_ok) {
    vlogprintf (E_WARNING, "Chat is not initialized yet\n");
    return -1;
  }
  memcpy (buf, P->encr_chat.first_key_sha, 16);
  return 0;
}
/* }}} */

/* {{{ Add user to chat */

void tgl_do_add_user_to_chat (struct tgl_state *TLS, tgl_peer_id_t chat_id, tgl_peer_id_t id, int limit, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_add_chat_user);
  out_int (tgl_get_peer_id (chat_id));

  assert (tgl_get_peer_type (id) == TGL_PEER_USER);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  out_int (limit);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

void tgl_do_del_user_from_chat (struct tgl_state *TLS, tgl_peer_id_t chat_id, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_delete_chat_user);
  out_int (tgl_get_peer_id (chat_id));

  assert (tgl_get_peer_type (id) == TGL_PEER_USER);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

/* }}} */

/* {{{ Add user to channel */

void tgl_do_channel_invite_user (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_invite_to_channel);
  out_int (CODE_input_channel);
  out_int (channel_id.peer_id);
  out_long (channel_id.access_hash);

  out_int (CODE_vector);
  out_int (1);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

void tgl_do_channel_kick_user (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_kick_from_channel);
  out_int (CODE_input_channel);
  out_int (channel_id.peer_id);
  out_long (channel_id.access_hash);

  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);

  out_int (CODE_bool_true);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

/* }}} */

/* {{{ Create secret chat */

void tgl_do_create_secret_chat (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  assert (tgl_get_peer_type (id) == TGL_PEER_USER);
  tgl_peer_t *U = tgl_peer_get (TLS, id);
  if (!U) {
    tgl_set_query_error (TLS, EINVAL, "Can not create secret chat with unknown user");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }

  tgl_do_create_encr_chat_request (TLS, tgl_get_peer_id (id), callback, callback_extra);
}
/* }}} */

/* {{{ Create group chat */

void tgl_do_create_group_chat (struct tgl_state *TLS, int users_num, tgl_peer_id_t ids[], const char *chat_topic, int chat_topic_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_create_chat);
  out_int (CODE_vector);
  out_int (users_num); // Number of users, currently we support only 1 user.
  int i;
  for (i = 0; i < users_num; i++) {
    tgl_peer_id_t id = ids[i];
    if (tgl_get_peer_type (id) != TGL_PEER_USER) {
      tgl_set_query_error (TLS, EINVAL, "Can not create chat with unknown user");
      if (callback) {
        callback (TLS, callback_extra, 0);
      }
      return;
    }
    out_int (CODE_input_user);
    out_int (tgl_get_peer_id (id));
    out_long (id.access_hash);
  }
  out_cstring (chat_topic, chat_topic_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Create channel */

void tgl_do_create_channel (struct tgl_state *TLS, int users_num, tgl_peer_id_t ids[], const char *chat_topic, int chat_topic_len, const char *about, int about_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_channels_create_channel);
  out_int (flags); // looks like 2 is disable non-admin messages
  out_cstring (chat_topic, chat_topic_len);
  out_cstring (about, about_len);
  //out_int (CODE_vector);
  //out_int (users_num); 
  int i;
  for (i = 0; i < users_num; i++) {
    tgl_peer_id_t id = ids[i];
    if (tgl_get_peer_type (id) != TGL_PEER_USER) {
      tgl_set_query_error (TLS, EINVAL, "Can not create chat with unknown user");
      if (callback) {
        callback (TLS, callback_extra, 0);
      }
      return;
    }
    out_int (CODE_input_user);
    out_int (tgl_get_peer_id (id));
    out_long (id.access_hash);
  }
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Delete msg */

static int delete_msg_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_affected_messages *DS_MAM = D;

  tgl_message_id_t *id = q->extra;
  q->extra = NULL;

  struct tgl_message *M = tgl_message_get (TLS, id);
  if (M) {
    bl_do_message_delete (TLS, &M->permanent_id);
  }
  tfree (id, sizeof (*id));

  int r = tgl_check_pts_diff (TLS, DS_LVAL (DS_MAM->pts), DS_LVAL (DS_MAM->pts_count));

  if (r > 0) {
    bl_do_set_pts (TLS, DS_LVAL (DS_MAM->pts));
  }

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
  }
  return 0;
}

static int delete_msg_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *,void *, int))(q->callback))(TLS, q->callback_extra, 0);
  }
  tfree (q->extra, sizeof (tgl_message_id_t));
  return 0;
}


static struct query_methods delete_msg_methods = {
  .on_answer = delete_msg_on_answer,
  .on_error = delete_msg_on_error,
  .type = TYPE_TO_PARAM(messages_affected_messages),
  .name = "delete message"
};

void tgl_do_delete_msg (struct tgl_state *TLS, tgl_message_id_t *_msg_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  tgl_message_id_t msg_id = *_msg_id;
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    msg_id = tgl_convert_temp_msg_id (TLS, msg_id);
  }
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }
  clear_packet ();
  if (msg_id.peer_type == TGL_PEER_CHANNEL) {
    out_int (CODE_channels_delete_messages);
    out_int (CODE_input_channel);
    out_int (msg_id.peer_id);
    out_long (msg_id.access_hash);

    out_int (CODE_vector);
    out_int (1);
    out_int (msg_id.id);
  } else {
    out_int (CODE_messages_delete_messages);
    out_int (CODE_vector);
    out_int (1);
    out_int (msg_id.id);
  }

  tgl_message_id_t *id = talloc (sizeof (*id));
  *id = msg_id;
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &delete_msg_methods, id, callback, callback_extra);
}
/* }}} */

/* {{{ Export card */

static int export_card_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_vector *DS_V = D;

  int n = DS_LVAL (DS_V->f1);

  int *r = talloc (4 * n);
  int i;
  for (i = 0; i < n; i++) {
    r[i] = *(int *)DS_V->f2[i];
  }

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, int, int *))q->callback) (TLS, q->callback_extra, 1, n, r);
  }
  tfree (r, 4 * n);
  return 0;
}

static struct query_methods export_card_methods = {
  .on_answer = export_card_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM_1(vector, TYPE_TO_PARAM (bare_int)),
  .name = "export card"
};

void tgl_do_export_card (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, int *card), void *callback_extra) {
  clear_packet ();
  out_int (CODE_contacts_export_card);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &export_card_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Import card */

static int import_card_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_user *U = tglf_fetch_alloc_user (TLS, D);

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_user *))q->callback) (TLS, q->callback_extra, 1, U);
  }
  return 0;
}

static struct query_methods import_card_methods = {
  .on_answer = import_card_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM (user),
  .name = "import card"
};

void tgl_do_import_card (struct tgl_state *TLS, int size, int *card, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra) {
  clear_packet ();
  out_int (CODE_contacts_import_card);
  out_int (CODE_vector);
  out_int (size);
  out_ints (card, size);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &import_card_methods, 0, callback, callback_extra);
}
/* }}} */

void tgl_do_start_bot (struct tgl_state *TLS, tgl_peer_id_t bot, tgl_peer_id_t chat, const char *str, int str_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_start_bot);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (bot));
  out_long (bot.access_hash);
  out_int (tgl_get_peer_id (chat));
  long long m;
  tglt_secure_random (&m, 8);
  out_long (m);
  out_cstring (str, str_len);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

/* {{{ Send typing */
static int send_typing_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods send_typing_methods = {
  .on_answer = send_typing_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "send typing"
};

void tgl_do_send_typing (struct tgl_state *TLS, tgl_peer_id_t id, enum tgl_typing_status status, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_ENCR_CHAT) {
    clear_packet ();
    out_int (CODE_messages_set_typing);
    out_peer_id (TLS, id);
    switch (status) {
    case tgl_typing_none:
    case tgl_typing_typing:
      out_int (CODE_send_message_typing_action);
      break;
    case tgl_typing_cancel:
      out_int (CODE_send_message_cancel_action);
      break;
    case tgl_typing_record_video:
      out_int (CODE_send_message_record_video_action);
      break;
    case tgl_typing_upload_video:
      out_int (CODE_send_message_upload_video_action);
      break;
    case tgl_typing_record_audio:
      out_int (CODE_send_message_record_audio_action);
      break;
    case tgl_typing_upload_audio:
      out_int (CODE_send_message_upload_audio_action);
      break;
    case tgl_typing_upload_photo:
      out_int (CODE_send_message_upload_photo_action);
      break;
    case tgl_typing_upload_document:
      out_int (CODE_send_message_upload_document_action);
      break;
    case tgl_typing_geo:
      out_int (CODE_send_message_geo_location_action);
      break;
    case tgl_typing_choose_contact:
      out_int (CODE_send_message_choose_contact_action);
      break;
    }
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_typing_methods, 0, callback, callback_extra);
  } else {
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
  }
}
/* }}} */

/* {{{ Extd query */
#ifndef DISABLE_EXTF
char *tglf_extf_print_ds (struct tgl_state *TLS, void *DS, struct paramed_type *T);

static int ext_query_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    char *buf = tglf_extf_print_ds (TLS, D, q->type);
    ((void (*)(struct tgl_state *, void *, int, char *))q->callback) (TLS, q->callback_extra, 1, buf);
  }
  tgl_paramed_type_free (q->type);
  return 0;
}

static struct query_methods ext_query_methods = {
  .on_answer = ext_query_on_answer,
  .on_error = q_list_on_error,
  .name = "ext query"
};

void tgl_do_send_extf (struct tgl_state *TLS, const char *data, int data_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *buf), void *callback_extra) {
  clear_packet ();

  ext_query_methods.type = tglf_extf_store (TLS, data, data_len);

  if (ext_query_methods.type) {
    tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &ext_query_methods, 0, callback, callback_extra);
  }
}
#else
void tgl_do_send_extf (struct tgl_state *TLS, const char *data, int data_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *buf), void *callback_extra) {
  assert (0);
}
#endif
/* }}} */

/* {{{ get messages */

static int get_messages_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_messages *DS_MM = D;

  int i;
  for (i = 0; i < DS_LVAL (DS_MM->users->cnt); i++) {
    tglf_fetch_alloc_user (TLS, DS_MM->users->data[i]);
  }
  for (i = 0; i < DS_LVAL (DS_MM->chats->cnt); i++) {
    tglf_fetch_alloc_chat (TLS, DS_MM->chats->data[i]);
  }

  struct tgl_message **ML;
  if (q->extra) {
    ML = talloc0 (sizeof (void *) * DS_LVAL (DS_MM->messages->cnt));
  } else {
    static struct tgl_message *M;
    M = NULL;
    ML = &M;
    assert (DS_LVAL (DS_MM->messages->cnt) <= 1);
  }
  for (i = 0; i < DS_LVAL (DS_MM->messages->cnt); i++) {
    ML[i] = tglf_fetch_alloc_message (TLS, DS_MM->messages->data[i], NULL);
  }
  if (q->callback) {
    if (q->extra) {
      ((void (*)(struct tgl_state *, void *, int, int, struct tgl_message **))q->callback)(TLS, q->callback_extra, 1, DS_LVAL (DS_MM->messages->cnt), ML);
    } else {
      if (DS_LVAL (DS_MM->messages->cnt) > 0) {
        ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback)(TLS, q->callback_extra, 1, *ML);
      } else {
        tgl_set_query_error (TLS, ENOENT, "no such message");
        ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback)(TLS, q->callback_extra, 0, NULL);
      }
    }
  }
  if (q->extra) {
    tfree (ML, sizeof (void *) * DS_LVAL (DS_MM->messages->cnt));
  }
  return 0;
}

static struct query_methods get_messages_methods = {
  .on_answer = get_messages_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM (messages_messages),
  .name = "get messages"
};

void tgl_do_get_message (struct tgl_state *TLS, tgl_message_id_t *_msg_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_message_id_t msg_id = *_msg_id;
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    msg_id = tgl_convert_temp_msg_id (TLS, msg_id);
  }
  if (msg_id.peer_type == TGL_PEER_TEMP_ID) {
    tgl_set_query_error (TLS, EINVAL, "unknown message");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }

  struct tgl_message *M = tgl_message_get (TLS, &msg_id);
  if (M) {
    if (callback) {
      callback (TLS, callback_extra, 1, M);
    }
    return;
  }

  clear_packet ();

  vlogprintf (E_ERROR, "id=%" INT64_PRINTF_MODIFIER "d\n", msg_id.id);
  out_int (CODE_messages_get_messages);
  out_int (CODE_vector);
  out_int (1);
  out_int (msg_id.id);


  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_messages_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ Export/import chat link */
static int export_chat_link_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_exported_chat_invite *DS_ECI = D;

  char *s = DS_STR_DUP (DS_ECI->link);

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, const char *))q->callback)(TLS, q->callback_extra, s ? 1 : 0, s);
  }
  tfree_str (s);
  return 0;
}


static struct query_methods export_chat_link_methods = {
  .on_answer = export_chat_link_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(exported_chat_invite),
  .name = "export chat link"
};

void tgl_do_export_chat_link (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *link), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_CHAT) {
    tgl_set_query_error (TLS, EINVAL, "Can only export chat link for chat");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }

  clear_packet ();
  out_int (CODE_messages_export_chat_invite);
  out_int (tgl_get_peer_id (id));

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &export_chat_link_methods, 0, callback, callback_extra);
}

void tgl_do_import_chat_link (struct tgl_state *TLS, const char *link, int len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  const char *l = link + len - 1;
  while (l >= link && *l != '/') {
    l --;
  }
  l ++;

  clear_packet ();
  out_int (CODE_messages_import_chat_invite);
  out_cstring (l, len - (l - link));

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}

/* }}} */

/* {{{ Export/import channel link */

void tgl_do_export_channel_link (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *link), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_CHANNEL) {
    tgl_set_query_error (TLS, EINVAL, "Can only export chat link for chat");
    if (callback) {
      callback (TLS, callback_extra, 0, NULL);
    }
    return;
  }

  clear_packet ();
  out_int (CODE_channels_export_invite);
  out_int (CODE_input_channel);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &export_chat_link_methods, 0, callback, callback_extra);
}

/* }}} */

/* {{{ set password */
static int set_password_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static int set_password_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  if (error_code == 400) {
    if (!strcmp (error, "PASSWORD_HASH_INVALID")) {
      vlogprintf (E_WARNING, "Bad old password\n");
      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
      }
      return 0;
    }
    if (!strcmp (error, "NEW_PASSWORD_BAD")) {
      vlogprintf (E_WARNING, "Bad new password (unchanged or equals hint)\n");
      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
      }
      return 0;
    }
    if (!strcmp (error, "NEW_SALT_INVALID")) {
      vlogprintf (E_WARNING, "Bad new salt\n");
      if (q->callback) {
        ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
      }
      return 0;
    }
  }
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
  }
  return 0;
}

static struct query_methods set_password_methods = {
  .on_answer = set_password_on_answer,
  .on_error = set_password_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "set password"
};

static void tgl_do_act_set_password (struct tgl_state *TLS, const char *current_password, int current_password_len, const char *new_password, int new_password_len, const char *current_salt, int current_salt_len, const char *new_salt, int new_salt_len, const char *hint, int hint_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  static char s[512];
  static unsigned char shab[32];

  assert (current_salt_len <= 128);
  assert (current_password_len <= 128);
  assert (new_salt_len <= 128);
  assert (new_password_len <= 128);

  out_int (CODE_account_update_password_settings);

  if (current_password_len && current_salt_len) {
    memcpy (s, current_salt, current_salt_len);
    memcpy (s + current_salt_len, current_password, current_password_len);
    memcpy (s + current_salt_len + current_password_len, current_salt, current_salt_len);

    TGLC_sha256 ((void *)s, 2 * current_salt_len + current_password_len, shab);
    out_cstring ((void *)shab, 32);
  } else {
    out_string ("");
  }

  out_int (CODE_account_password_input_settings);
  if (new_password_len) {
    out_int (1);

    static char d[256];
    memcpy (d, new_salt, new_salt_len);

    int l = new_salt_len;
    tglt_secure_random (d + l, 16);
    l += 16;
    memcpy (s, d, l);

    memcpy (s + l, new_password, new_password_len);
    memcpy (s + l + new_password_len, d, l);

    TGLC_sha256 ((void *)s, 2 * l + new_password_len, shab);

    out_cstring (d, l);
    out_cstring ((void *)shab, 32);
    out_cstring (hint, hint_len);
  } else {
    out_int (0);
  }


  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_password_methods, 0, callback, callback_extra);
}

struct change_password_extra {
  char *current_password;
  char *new_password;
  char *current_salt;
  char *new_salt;
  char *hint;
  int current_password_len;
  int new_password_len;
  int current_salt_len;
  int new_salt_len;
  int hint_len;
  void (*callback)(struct tgl_state *, void *, int);
  void *callback_extra;
};

void tgl_on_new_pwd (struct tgl_state *TLS, const char *pwd[], void *_T);
void tgl_on_new2_pwd (struct tgl_state *TLS, const char *pwd, void *_T) {
  struct change_password_extra *E = _T;
  if (strlen (pwd) != (size_t)E->new_password_len || memcmp (E->new_password, pwd, E->new_password_len)) {
    tfree (E->new_password, E->new_password_len);
    E->new_password = NULL;
    E->new_password_len = 0;
    vlogprintf (E_ERROR, "passwords do not match\n");
    TLS->callback.get_values (TLS, tgl_new_password, "new password: ", 2, tgl_on_new_pwd, E);
    return;
  }
  tgl_do_act_set_password (TLS, E->current_password, E->current_password_len,
                                E->new_password, E->new_password_len,
                                E->current_salt, E->current_salt_len,
                                E->new_salt, E->new_salt_len,
                                E->hint, E->hint_len,
                                E->callback, E->callback_extra);

  tfree (E->current_password, E->current_password_len);
  tfree (E->new_password, E->new_password_len);
  tfree (E->current_salt, E->current_salt_len);
  tfree (E->new_salt, E->new_salt_len);
  tfree_str (E->hint);
  tfree (E, sizeof (*E));
}

void tgl_on_new_pwd (struct tgl_state *TLS, const char *pwd[], void *_T) {
  struct change_password_extra *E = _T;
  E->new_password_len = strlen (pwd[0]);
  E->new_password = tmemdup (pwd[0], E->new_password_len);
  tgl_on_new2_pwd(TLS, pwd[1], E);
}

void tgl_on_old_pwd (struct tgl_state *TLS, const char *pwd[], void *_T) {
  struct change_password_extra *E = _T;
  E->current_password_len = strlen (pwd[0]);
  E->current_password = tmemdup (pwd[0], E->current_password_len);
  tgl_on_new_pwd(TLS, pwd + 1, E);
}

static int set_get_password_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_account_password *DS_AP = D;

  char *new_hint = q->extra;

  struct change_password_extra *E = talloc0 (sizeof (*E));

  if (DS_AP->current_salt) {
    E->current_salt_len = DS_AP->current_salt->len;
    E->current_salt = tmemdup (DS_AP->current_salt->data, E->current_salt_len);
  }
  if (DS_AP->new_salt) {
    E->new_salt_len = DS_AP->new_salt->len;
    E->new_salt = tmemdup (DS_AP->new_salt->data, E->new_salt_len);
  }

  if (new_hint) {
    E->hint_len = strlen (new_hint);
    E->hint = new_hint;
  }

  E->callback = q->callback;
  E->callback_extra = q->callback_extra;

  if (DS_AP->magic == CODE_account_no_password) {
    TLS->callback.get_values (TLS, tgl_new_password, "new password: ", 2, tgl_on_new_pwd, E);
  } else {
    static char s[512];
    snprintf (s, 511, "old password (hint %.*s): ", DS_RSTR (DS_AP->hint));
    TLS->callback.get_values (TLS, tgl_cur_and_new_password, s, 3, tgl_on_old_pwd, E);
  }
  return 0;
}

static struct query_methods set_get_password_methods = {
  .on_answer = set_get_password_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(account_password),
  .name = "get password"
};

void tgl_do_set_password (struct tgl_state *TLS, const char *hint, int hint_len, void (*callback)(struct tgl_state *TLS, void *extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_account_get_password);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_get_password_methods, hint ? tstrndup (hint, hint_len) : NULL, callback, callback_extra);
}

/* }}} */

/* {{{ check password */
static int check_password_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  if (error_code == 400) {
    vlogprintf (E_ERROR, "bad password\n");
    tgl_do_check_password (TLS, q->callback, q->callback_extra);
    return 0;
  }
  TLS->locks ^= TGL_LOCK_PASSWORD;
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 0);
  }
  return 0;
}

static int check_password_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  TLS->locks ^= TGL_LOCK_PASSWORD;
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods check_password_methods = {
  .on_answer = check_password_on_answer,
  .on_error = check_password_on_error,
  .type = TYPE_TO_PARAM(auth_authorization),
  .name = "check password"
};


struct check_password_extra {
  char *current_salt;
  int current_salt_len;
  void (*callback)(struct tgl_state *, void *, int);
  void *callback_extra;
};

static void tgl_pwd_got (struct tgl_state *TLS, const char *pwd[], void *_T) {
  struct check_password_extra *E = _T;

  clear_packet ();
  static char s[512];
  static unsigned char shab[32];

  assert (E->current_salt_len <= 128);
  assert (strlen (pwd[0]) <= 128);

  out_int (CODE_auth_check_password);

  if (pwd[0] && E->current_salt_len) {
    int l = E->current_salt_len;
    memcpy (s, E->current_salt, l);

    int r = strlen (pwd[0]);
    strcpy (s + l, pwd[0]);

    memcpy (s + l + r, E->current_salt, l);

    TGLC_sha256 ((void *)s, 2 * l + r, shab);
    out_cstring ((void *)shab, 32);
  } else {
    out_string ("");
  }

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &check_password_methods, 0, E->callback, E->callback_extra);

  tfree (E->current_salt, E->current_salt_len);
  tfree (E, sizeof (*E));
}

static int check_get_password_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  TLS->locks ^= TGL_LOCK_PASSWORD;
  tgl_set_query_error (TLS, EPROTO, "RPC_CALL_FAIL %d: %.*s", error_code, error_len, error);
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 0);
  }
  return 0;
}

static int check_get_password_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_account_password *DS_AP = D;

  if (DS_AP->magic == CODE_account_no_password) {
    TLS->locks ^= TGL_LOCK_PASSWORD;
    return 0;
  }
  static char s[512];
  snprintf (s, 511, "type password (hint %.*s): ", DS_RSTR (DS_AP->hint));

  struct check_password_extra *E = talloc0 (sizeof (*E));

  if (DS_AP->current_salt) {
    E->current_salt_len = DS_AP->current_salt->len;
    E->current_salt = tmemdup (DS_AP->current_salt->data, E->current_salt_len);
  }

  E->callback = q->callback;
  E->callback_extra = q->callback_extra;

  TLS->callback.get_values (TLS, tgl_cur_password, s, 1, tgl_pwd_got, E);
  return 0;
}

static struct query_methods check_get_password_methods = {
  .on_answer = check_get_password_on_answer,
  .on_error = check_get_password_on_error,
  .type = TYPE_TO_PARAM(account_password),
  .name = "get password"
};

void tgl_do_check_password (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_account_get_password);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &check_get_password_methods, NULL, callback, callback_extra);
}

/* }}} */

/* {{{ send broadcast */
void tgl_do_send_broadcast (struct tgl_state *TLS, int num, tgl_peer_id_t peer_id[], const char *text, int text_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *extra, int success, int num, struct tgl_message *ML[]), void *callback_extra) {

  assert (num <= 1000);

  struct messages_send_extra *E = talloc0 (sizeof (*E));
  E->multi = 1;
  E->count = num;
  E->list = talloc (sizeof (tgl_message_id_t) * num);

  int date = time (0);
  struct tl_ds_message_media TDSM;
  TDSM.magic = CODE_message_media_empty;

  int i;
  for (i = 0; i < num; i++) {
    assert (tgl_get_peer_type (peer_id[i]) == TGL_PEER_USER);

    int disable_preview = flags & TGL_SEND_MSG_FLAG_DISABLE_PREVIEW;
    if (!(flags & TGL_SEND_MSG_FLAG_ENABLE_PREVIEW) && TLS->disable_link_preview) {
      disable_preview = 1;
    }
    if (disable_preview) {
      disable_preview = TGLMF_DISABLE_PREVIEW;
    }

    struct tgl_message_id id = tgl_peer_id_to_random_msg_id (peer_id[i]);
    E->list[i] = id;

    tgl_peer_id_t from_id = TLS->our_id;
    bl_do_edit_message (TLS, &id, &from_id, &peer_id[i], NULL, NULL, &date, text, text_len, &TDSM, NULL, NULL, NULL, NULL, TGLMF_UNREAD | TGLMF_OUT | TGLMF_PENDING | TGLMF_CREATE | TGLMF_CREATED | disable_preview);
  }

  clear_packet ();
  out_int (CODE_messages_send_broadcast);
  out_int (CODE_vector);
  out_int (num);
  for (i = 0; i < num; i++) {
    assert (tgl_get_peer_type (peer_id[i]) == TGL_PEER_USER);

    out_int (CODE_input_user);
    out_int (tgl_get_peer_id (peer_id[i]));
    out_long (peer_id[i].access_hash);
  }

  out_int (CODE_vector);
  out_int (num);
  for (i = 0; i < num; i++) {
    out_long (E->list[i].id);
  }
  out_cstring (text, text_len);

  out_int (CODE_message_media_empty);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, E, callback, callback_extra);
}
/* }}} */

/* {{{ block user */
static int block_user_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods block_user_methods = {
  .on_answer = block_user_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM (bool),
  .name = "block user"
};

void tgl_do_block_user (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_USER) {
    tgl_set_query_error (TLS, EINVAL, "id should be user id");
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }

  clear_packet ();

  out_int (CODE_contacts_block);
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &block_user_methods, 0, callback, callback_extra);
}

void tgl_do_unblock_user (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  if (tgl_get_peer_type (id) != TGL_PEER_USER) {
    tgl_set_query_error (TLS, EINVAL, "id should be user id");
    if (callback) {
      callback (TLS, callback_extra, 0);
    }
    return;
  }

  clear_packet ();

  out_int (CODE_contacts_unblock);
  
  out_int (CODE_input_user);
  out_int (tgl_get_peer_id (id));
  out_long (id.access_hash);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &block_user_methods, 0, callback, callback_extra);
}
/* }}} */

/* {{{ get terms of service */
static int get_tos_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_help_terms_of_service *DS_T = D;
  int l = DS_T->text->len;
  char *s = talloc (l + 1);
  char *str = DS_T->text->data;
  int p = 0;
  int pp = 0;
  while (p < l) {
    if (*str == '\\' && p < l - 1) {
      str ++;
      p ++;
      switch (*str) {
      case 'n':
        s[pp ++] = '\n';
        break;
      case 't':
        s[pp ++] = '\t';
        break;
      case 'r':
        s[pp ++] = '\r';
        break;
      default:
        s[pp ++] = *str;
      }
      str ++;
      p ++;
    } else {
      s[pp ++] = *str;
      str ++;
      p ++;
    }
  }
  s[pp] = 0;

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, char *))q->callback)(TLS, q->callback_extra, 1, s);
  }
  tfree (s, l + 1);
  return 0;
}

static struct query_methods get_tos_methods = {
  .on_answer = get_tos_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM (help_terms_of_service),
  .name = "get tos"
};

void tgl_do_get_terms_of_service (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *ans), void *callback_extra) {
  clear_packet ();

  out_int (CODE_help_get_terms_of_service);
  out_string ("");
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_tos_methods, 0, callback, callback_extra);
}
/* }}} */

void tgl_do_upgrade_group (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();

  out_int (CODE_messages_migrate_chat);
  out_int (tgl_get_peer_id (id));
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_msgs_methods, 0, callback, callback_extra);
}


static void set_flag_4 (struct tgl_state *TLS, void *_D, int success) {
  struct tgl_dc *D = _D;
  assert (success);
  D->flags |= 4;

  TLS->timer_methods->insert (D->ev, TLS->temp_key_expire_time * 0.9);
}

static int send_bind_temp_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_dc *DC = q->extra;
  DC->flags |= 2;
  tgl_do_help_get_config_dc (TLS, DC, set_flag_4, DC);
  vlogprintf (E_DEBUG, "Bind successful in dc %d\n", DC->id);
  return 0;
}

static int send_bind_on_error (struct tgl_state *TLS, struct query *q, int error_code, int l, const char *error) {
  vlogprintf (E_WARNING, "bind: error %d: %.*s\n", error_code, l, error);
  if (error_code == 400) {
    return -11;
  }
  return 0;
}

static struct query_methods send_bind_temp_methods = {
  .on_answer = send_bind_temp_on_answer,
  .on_error = send_bind_on_error,
  .type = TYPE_TO_PARAM (bool),
  .name = "bind temp auth key"
};

void tgl_do_send_bind_temp_key (struct tgl_state *TLS, struct tgl_dc *D, long long nonce, int expires_at, void *data, int len, long long msg_id) {
  clear_packet ();
  out_int (CODE_auth_bind_temp_auth_key);
  out_long (D->auth_key_id);
  out_long (nonce);
  out_int (expires_at);
  out_cstring (data, len);
  struct query *q = tglq_send_query_ex (TLS, D, packet_ptr - packet_buffer, packet_buffer, &send_bind_temp_methods, D, 0, 0, 2);
  assert (q->msg_id == msg_id);
}

static int update_status_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback) (TLS, q->callback_extra, 1);
  }
  return 0;
}

static struct query_methods update_status_methods = {
  .on_answer = update_status_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "update status"
};

void tgl_do_update_status (struct tgl_state *TLS, int online, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  clear_packet ();
  out_int (CODE_account_update_status);
  out_int (online ? CODE_bool_false : CODE_bool_true);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &update_status_methods, 0, callback, callback_extra);
}


void tgl_do_request_exchange (struct tgl_state *TLS, struct tgl_secret_chat *E) {
  assert (0);
  exit (2);
}

void tgl_do_accept_exchange (struct tgl_state *TLS, struct tgl_secret_chat *E, long long exchange_id, unsigned char ga[]) {
  assert (0);
  exit (2);
}

void tgl_do_confirm_exchange (struct tgl_state *TLS, struct tgl_secret_chat *E, int sen_nop) {
  assert (0);
  exit (2);
}

void tgl_do_commit_exchange (struct tgl_state *TLS, struct tgl_secret_chat *E, unsigned char gb[]) {
  assert (0);
  exit (2);
}

void tgl_do_abort_exchange (struct tgl_state *TLS, struct tgl_secret_chat *E) {
  assert (0);
  exit (2);
}

void tgl_started_cb (struct tgl_state *TLS, void *arg, int success) {
  if (!success) {
    vlogprintf (E_ERROR, "login problem: error #%d (%s)\n", TLS->error_code, TLS->error);
    if (TLS->callback.on_failed_login) {
      TLS->callback.on_failed_login (TLS);
    } else {
      assert (success);
    }
    return;
  }
  TLS->started = 1;
  if (TLS->callback.started) {
    TLS->callback.started (TLS);
  }
}

void tgl_export_auth_callback (struct tgl_state *TLS, void *arg, int success) {
  if (!success) {
    vlogprintf (E_ERROR, "login problem: error #%d (%s)\n", TLS->error_code, TLS->error);
    if (TLS->callback.on_failed_login) {
      TLS->callback.on_failed_login (TLS);
    } else {
      assert (success);
    }
    return;
  }
  int i;
  for (i = 0; i <= TLS->max_dc_num; i++) if (TLS->DC_list[i] && !tgl_signed_dc (TLS, TLS->DC_list[i])) {
    return;
  }
  if (TLS->callback.logged_in) {
    TLS->callback.logged_in (TLS);
  }

  tglm_send_all_unsent (TLS);
  tgl_do_get_difference (TLS, 0, tgl_started_cb, 0);
}

void tgl_export_all_auth (struct tgl_state *TLS) {
  int i;
  int ok = 1;
  for (i = 0; i <= TLS->max_dc_num; i++) if (TLS->DC_list[i] && !tgl_signed_dc (TLS, TLS->DC_list[i])) {
    tgl_do_export_auth (TLS, i, tgl_export_auth_callback, TLS->DC_list[i]);
    ok = 0;
  }
  if (ok) {
    if (TLS->callback.logged_in) {
      TLS->callback.logged_in (TLS);
    }

    tglm_send_all_unsent (TLS);
    tgl_do_get_difference (TLS, 0, tgl_started_cb, 0);
  }
}

struct sign_up_extra {
  char *phone;
  char *hash;
  char *first_name;
  char *last_name;
  int phone_len;
  int hash_len;
  int first_name_len;
  int last_name_len;
};

void tgl_sign_in_code (struct tgl_state *TLS, const char *code[], void *_T);
void tgl_sign_in_result (struct tgl_state *TLS, void *_T, int success, struct tgl_user *U) {
  struct sign_up_extra *E = _T;
  if (success) {
    tfree (E->phone, E->phone_len);
    tfree (E->hash, E->hash_len);
    tfree (E, sizeof (*E));
  } else {
    vlogprintf (E_ERROR, "incorrect code\n");
    TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_in_code, E);
    return;
  }
  tgl_export_all_auth (TLS);
}

void tgl_sign_in_code (struct tgl_state *TLS, const char *code[], void *_T) {
  struct sign_up_extra *E = _T;
  if (!strcmp (code[0], "call")) {
    tgl_do_phone_call (TLS, E->phone, E->phone_len, E->hash, E->hash_len, 0, 0);
    TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_in_code, E);
    return;
  }

  tgl_do_send_code_result (TLS, E->phone, E->phone_len, E->hash, E->hash_len, code[0], strlen (code[0]), tgl_sign_in_result, E);
}

void tgl_sign_up_code (struct tgl_state *TLS, const char *code[], void *_T);
void tgl_sign_up_result (struct tgl_state *TLS, void *_T, int success, struct tgl_user *U) {
  struct sign_up_extra *E = _T;
  if (success) {
    tfree (E->phone, E->phone_len);
    tfree (E->hash, E->hash_len);
    tfree (E->first_name, E->first_name_len);
    tfree (E->last_name, E->last_name_len);
    tfree (E, sizeof (*E));
  } else {
    vlogprintf (E_ERROR, "incorrect code\n");
    TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_up_code, E);
    return;
  }
  tgl_export_all_auth (TLS);
}

void tgl_sign_up_code (struct tgl_state *TLS, const char *code[], void *_T) {
  struct sign_up_extra *E = _T;
  if (!strcmp (code[0], "call")) {
    tgl_do_phone_call (TLS, E->phone, E->phone_len, E->hash, E->hash_len, 0, 0);
    TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_up_code, E);
    return;
  }

  tgl_do_send_code_result_auth (TLS, E->phone, E->phone_len, E->hash, E->hash_len, code[0], strlen (code[0]), E->first_name, E->first_name_len, E->last_name, E->last_name_len, tgl_sign_up_result, E);
}


void tgl_set_last_name (struct tgl_state *TLS, const char *last_name, struct sign_up_extra *E) {
  E->last_name_len = strlen (last_name);
  E->last_name = tmemdup (last_name, E->last_name_len);
}

int tgl_set_first_name (struct tgl_state *TLS, const char *first_name, struct sign_up_extra *E) {
  if (strlen (first_name) < 1) {
    return -1;
  }

  E->first_name_len = strlen (first_name);
  E->first_name = tmemdup (first_name, E->first_name_len);
  return 0;
}

void tgl_register_cb (struct tgl_state *TLS, const char *rinfo[], void *_T) {
  struct sign_up_extra *E = _T;
  const char *yn = rinfo[0];
  if (strlen (yn) > 1) {
    TLS->callback.get_values (TLS, tgl_register_info, "registration info:", 3, tgl_register_cb, E);
  } else if (strlen (yn) == 0 || *yn == 'y' || *yn == 'Y') {
    if (!tgl_set_first_name(TLS, rinfo[1], E)) {
      tgl_set_last_name(TLS, rinfo[2], E);
      TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_up_code, E);
    }
    else {
      TLS->callback.get_values (TLS, tgl_register_info, "registration info:", 3, tgl_register_cb, E);
    }
  } else if (*yn == 'n' || *yn == 'N') {
    vlogprintf (E_ERROR, "stopping registration");
    tfree (E->phone, E->phone_len);
    tfree (E->hash, E->hash_len);
    tfree (E, sizeof (*E));
    tgl_login (TLS);
  } else {
    TLS->callback.get_values (TLS, tgl_register_info, "registration info:", 3, tgl_register_cb, E);
  }
}

void tgl_sign_in_phone (struct tgl_state *TLS, const char *phone[], void *arg);
void tgl_sign_in_phone_cb (struct tgl_state *TLS, void *extra, int success, int registered, const char *mhash) {
  struct sign_up_extra *E = extra;
  if (!success) {
    vlogprintf (E_ERROR, "Incorrect phone number\n");

    tfree (E->phone, E->phone_len);
    tfree (E, sizeof (*E));
    TLS->callback.get_values (TLS, tgl_phone_number, "phone number:", 1, tgl_sign_in_phone, NULL);
    return;
  }

  E->hash_len = strlen (mhash);
  E->hash = tmemdup (mhash, E->hash_len);

  if (registered) {
    TLS->callback.get_values (TLS, tgl_code, "code ('call' for phone call):", 1, tgl_sign_in_code, E);
  } else {
    TLS->callback.get_values (TLS, tgl_register_info, "registration info:", 3, tgl_register_cb, E);
  }
}

void tgl_sign_in_phone (struct tgl_state *TLS, const char *phone[], void *arg) {
  struct sign_up_extra *E = talloc0 (sizeof (*E));
  E->phone_len = strlen (phone[0]);
  E->phone = tmemdup (phone[0], E->phone_len);

  tgl_do_send_code (TLS, E->phone, E->phone_len, tgl_sign_in_phone_cb, E);
}

void tgl_bot_hash_cb (struct tgl_state *TLS, const char *code[], void *arg);

void tgl_sign_in_bot_cb (struct tgl_state *TLS, void *_T, int success, struct tgl_user *U) {
  if (!success) {
    vlogprintf (E_ERROR, "incorrect bot hash\n");
    TLS->callback.get_values (TLS, tgl_bot_hash, "bot hash:", 1, tgl_bot_hash_cb, NULL);
    return;
  }
  tgl_export_all_auth (TLS);
}

void tgl_bot_hash_cb (struct tgl_state *TLS, const char *code[], void *arg) {
  tgl_do_send_bot_auth (TLS, code[0], strlen (code[0]), tgl_sign_in_bot_cb, NULL);
}

void tgl_sign_in (struct tgl_state *TLS) {
  if (!tgl_signed_dc (TLS, TLS->DC_working)) {
    if (TLS->is_bot) {
      TLS->callback.get_values (TLS, tgl_bot_hash, "bot hash:", 1, tgl_bot_hash_cb, NULL);
    } else {
      TLS->callback.get_values (TLS, tgl_phone_number, "phone number:", 1, tgl_sign_in_phone, NULL);
    }
  } else {
    tgl_export_all_auth (TLS);
  }
}

static void check_authorized (struct tgl_state *TLS, void *arg) {
  int i;
  int ok = 1;
  for (i = 0; i <= TLS->max_dc_num; i++) {
    if (TLS->DC_list[i]) {
      tgl_dc_authorize (TLS, TLS->DC_list[i]);
    }
  }
  for (i = 0; i <= TLS->max_dc_num; i++) {
    if (TLS->DC_list[i] && !tgl_signed_dc (TLS, TLS->DC_list[i]) && !tgl_authorized_dc (TLS, TLS->DC_list[i])) {
      ok = 0;
      break;
    }
  }

  if (ok) {
    TLS->timer_methods->free (TLS->ev_login);
    TLS->ev_login = NULL;
    tgl_sign_in (TLS);
  } else {
    TLS->timer_methods->insert (TLS->ev_login, 0.1);
  }
}

void tgl_login (struct tgl_state *TLS) {
  int i;
  int ok = 1;
  for (i = 0; i <= TLS->max_dc_num; i++) {
    if (TLS->DC_list[i] && !tgl_signed_dc (TLS, TLS->DC_list[i]) && !tgl_authorized_dc (TLS, TLS->DC_list[i])) {
      ok = 0;
      break;
    }
  }

  if (!ok) {
    TLS->ev_login = TLS->timer_methods->alloc (TLS, check_authorized, NULL);
    TLS->timer_methods->insert (TLS->ev_login, 0.1);
  } else {
    tgl_sign_in (TLS);
  }
}


static int callback (struct tgl_state *TLS, struct query *q, void *D) {
   if (q->callback) {
     ((void (*)(struct tgl_state *, void *))(q->callback)) (TLS, q->callback_extra);
  }
  return 0;
}

static int send_change_code_on_answer (struct tgl_state *TLS, struct query *q, void *D) {

  struct tl_ds_account_sent_change_phone_code *DS_ASCPC= D;

  char *phone_code_hash = DS_STR_DUP (DS_ASCPC->phone_code_hash);

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, const char *))(q->callback)) (TLS, q->callback_extra, 1, phone_code_hash);
  }
  tfree_str (phone_code_hash);
  return 0;
}

struct change_phone_extra {
  char *phone;
  char *hash;
  char *first_name;
  char *last_name;
  int phone_len;
  int hash_len;
  int first_name_len;
  int last_name_len;
  void (*callback)(struct tgl_state *TLS, void *extra, int success);
  void *callback_extra;
};


static struct query_methods set_phone_methods  = {
  .on_answer = callback,
  .on_error = sign_in_on_error,
  .type = TYPE_TO_PARAM(user),
  .name = "set phone"
};

static struct query_methods send_change_code_methods  = {
  .on_answer = send_change_code_on_answer,
  .on_error = q_list_on_error,
  .type = TYPE_TO_PARAM(account_sent_change_phone_code),
  .name = "send change phone code"
};

void tgl_set_number_code (struct tgl_state *TLS, const char *code[], void *_T);
void tgl_set_number_result (struct tgl_state *TLS, void *_T, int success, struct tgl_user *U) {
  struct change_phone_extra *E = _T;
  if (success) {
    if (E->callback) {
      E->callback (TLS, E->callback_extra, 1);
    }
    tfree (E->phone, E->phone_len);
    tfree (E->hash, E->hash_len);
    tfree (E, sizeof (*E));
  } else {
    vlogprintf (E_ERROR, "incorrect code\n");
    TLS->callback.get_values (TLS, tgl_code, "code:", 1, tgl_set_number_code, E);
  }
}

void tgl_set_number_code (struct tgl_state *TLS, const char *code[], void *_T) {
  struct change_phone_extra *E = _T;

  clear_packet ();
  out_int (CODE_account_change_phone);
  out_cstring (E->phone, E->phone_len);
  out_cstring (E->hash, E->hash_len);
  out_cstring (code[0], strlen (code[0]));
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &set_phone_methods, 0, tgl_set_number_result, E);
}


void tgl_set_phone_number_cb (struct tgl_state *TLS, void *extra, int success, const char *mhash) {
  struct change_phone_extra *E = extra;
  if (!success) {
    vlogprintf (E_ERROR, "Incorrect phone number\n");
    if (E->callback) {
      E->callback (TLS, E->callback_extra, 0);
    }
    tfree (E->phone, E->phone_len);
    tfree (E, sizeof (*E));
    return;
  }

  E->hash_len = strlen (mhash);
  E->hash = tmemdup (mhash, E->hash_len);

  TLS->callback.get_values (TLS, tgl_code, "code:", 1, tgl_set_number_code, E);
}

void tgl_do_set_phone_number (struct tgl_state *TLS, const char *phonenumber, int phonenumber_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra) {
  struct change_phone_extra *E = talloc0 (sizeof (*E));
  E->phone_len = phonenumber_len;
  E->phone = tmemdup (phonenumber, E->phone_len);

  clear_packet ();
  tgl_do_insert_header (TLS);
  out_int (CODE_account_send_change_phone_code);
  out_cstring (E->phone, E->phone_len);
  E->callback = callback;
  E->callback_extra = callback_extra;

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_change_code_methods, NULL, tgl_set_phone_number_cb, E);
}
