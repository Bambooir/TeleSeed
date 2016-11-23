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

    Copyright Vitaly Valtman 2014-2015
*/
#ifndef __TGL_H__
#define __TGL_H__

#include "crypto/bn.h"
#include "tgl-layout.h"
#include <string.h>
#include <stdlib.h>

#define TGL_MAX_DC_NUM 100
#define TG_SERVER_1 "149.154.175.50"
#define TG_SERVER_2 "149.154.167.51"
#define TG_SERVER_3 "149.154.175.100"
#define TG_SERVER_4 "149.154.167.91"
#define TG_SERVER_5 "149.154.171.5"
#define TG_SERVER_IPV6_1 "2001:b28:f23d:f001::a"
#define TG_SERVER_IPV6_2 "2001:67c:4e8:f002::a"
#define TG_SERVER_IPV6_3 "2001:b28:f23d:f003::a"
#define TG_SERVER_IPV6_4 "2001:67c:4e8:f004::a"
#define TG_SERVER_IPV6_5 "2001:b28:f23f:f005::a"
#define TG_SERVER_DEFAULT 2

#define TG_SERVER_TEST_1 "149.154.175.10"
#define TG_SERVER_TEST_2 "149.154.167.40"
#define TG_SERVER_TEST_3 "149.154.175.117"
#define TG_SERVER_TEST_IPV6_1 "2001:b28:f23d:f001::e"
#define TG_SERVER_TEST_IPV6_2 "2001:67c:4e8:f002::e"
#define TG_SERVER_TEST_IPV6_3 "2001:b28:f23d:f003::e"
#define TG_SERVER_TEST_DEFAULT 2

#define TGL_VERSION "2.1.0"

#define TGL_ENCRYPTED_LAYER 17
#define TGL_SCHEME_LAYER 45

struct connection;
struct mtproto_methods;
struct tgl_session;
struct tgl_dc;

#define TGL_UPDATE_CREATED 1
#define TGL_UPDATE_DELETED 2
#define TGL_UPDATE_PHONE 4
#define TGL_UPDATE_CONTACT 8
#define TGL_UPDATE_PHOTO 16
#define TGL_UPDATE_BLOCKED 32
#define TGL_UPDATE_REAL_NAME 64
#define TGL_UPDATE_NAME 128
#define TGL_UPDATE_REQUESTED 256
#define TGL_UPDATE_WORKING 512
#define TGL_UPDATE_FLAGS 1024
#define TGL_UPDATE_TITLE 2048
#define TGL_UPDATE_ADMIN 4096
#define TGL_UPDATE_MEMBERS 8192
#define TGL_UPDATE_ACCESS_HASH 16384
#define TGL_UPDATE_USERNAME (1 << 15)

/*struct tgl_allocator {
  void *(*alloc)(size_t size);
  void *(*realloc)(void *ptr, size_t old_size, size_t size);
  void (*free)(void *ptr, int size);
  void (*check)(void);
  void (*exists)(void *ptr, int size);
};*/
struct tgl_allocator;
extern struct tgl_allocator tgl_allocator_release;
extern struct tgl_allocator tgl_allocator_debug;
struct tgl_state;

enum tgl_value_type {
    tgl_phone_number,           // user phone number
    tgl_code,                   // telegram login code, or 'call' for phone call request
    tgl_register_info,          // "Y/n" register?, first name, last name
    tgl_new_password,           // new pass, confirm new pass
    tgl_cur_and_new_password,   // curr pass, new pass, confirm new pass
    tgl_cur_password,           // current pass
    tgl_bot_hash
};

struct tgl_update_callback {
  void (*new_msg)(struct tgl_state *TLS, struct tgl_message *M);
  void (*marked_read)(struct tgl_state *TLS, int num, struct tgl_message *list[]);
  void (*logprintf)(const char *format, ...)  __attribute__ ((format (__printf__, 1, 2)));
  void (*get_values)(struct tgl_state *TLS, enum tgl_value_type type, const char *prompt, int num_values,
          void (*callback)(struct tgl_state *TLS, const char *string[], void *arg), void *arg);
  void (*logged_in)(struct tgl_state *TLS);
  void (*started)(struct tgl_state *TLS);
  void (*type_notification)(struct tgl_state *TLS, struct tgl_user *U, enum tgl_typing_status status);
  void (*type_in_chat_notification)(struct tgl_state *TLS, struct tgl_user *U, struct tgl_chat *C, enum tgl_typing_status status);
  void (*type_in_secret_chat_notification)(struct tgl_state *TLS, struct tgl_secret_chat *E);
  void (*status_notification)(struct tgl_state *TLS, struct tgl_user *U);
  void (*user_registered)(struct tgl_state *TLS, struct tgl_user *U);
  void (*user_activated)(struct tgl_state *TLS, struct tgl_user *U);
  void (*new_authorization)(struct tgl_state *TLS, const char *device, const char *location);
  void (*chat_update)(struct tgl_state *TLS, struct tgl_chat *C, unsigned flags);
  void (*channel_update)(struct tgl_state *TLS, struct tgl_channel *C, unsigned flags);
  void (*user_update)(struct tgl_state *TLS, struct tgl_user *C, unsigned flags);
  void (*secret_chat_update)(struct tgl_state *TLS, struct tgl_secret_chat *C, unsigned flags);
  void (*msg_receive)(struct tgl_state *TLS, struct tgl_message *M);
  void (*our_id)(struct tgl_state *TLS, tgl_peer_id_t id);
  void (*notification)(struct tgl_state *TLS, const char *type, const char *message);
  void (*user_status_update)(struct tgl_state *TLS, struct tgl_user *U);
  char *(*create_print_name) (struct tgl_state *TLS, tgl_peer_id_t id, const char *a1, const char *a2, const char *a3, const char *a4);
  void (*on_failed_login) (struct tgl_state *TLS);
};

struct tgl_net_methods {
  int (*write_out) (struct connection *c, const void *data, int len);
  int (*read_in) (struct connection *c, void *data, int len);
  int (*read_in_lookup) (struct connection *c, void *data, int len);
  void (*flush_out) (struct connection *c);
  void (*incr_out_packet_num) (struct connection *c);
  void (*free) (struct connection *c);
  struct tgl_dc *(*get_dc) (struct connection *c);
  struct tgl_session *(*get_session) (struct connection *c);

  struct connection *(*create_connection) (struct tgl_state *TLS, const char *host, int port, struct tgl_session *session, struct tgl_dc *dc, struct mtproto_methods *methods);
};

struct mtproto_methods {
  int (*ready) (struct tgl_state *TLS, struct connection *c);
  int (*close) (struct tgl_state *TLS, struct connection *c);
  int (*execute) (struct tgl_state *TLS, struct connection *c, int op, int len);
};

struct tgl_timer;
struct tree_random_id;
struct tree_temp_id;

struct tgl_timer_methods {
  struct tgl_timer *(*alloc) (struct tgl_state *TLS, void (*cb)(struct tgl_state *TLS, void *arg), void *arg);
  void (*insert) (struct tgl_timer *t, double timeout);
  void (*remove) (struct tgl_timer *t);
  void (*free) (struct tgl_timer *t);
};

#define E_ERROR 0
#define E_WARNING 1
#define E_NOTICE 2
#define E_DEBUG 6

#define TGL_LOCK_DIFF 1
#define TGL_LOCK_PASSWORD 2

#define TGL_MAX_RSA_KEYS_NUM 10
// Do not modify this structure, unless you know what you do

#pragma pack(push,4)
struct tgl_state {
  tgl_peer_id_t our_id;
  int encr_root;
  unsigned char *encr_prime;
  TGLC_bn *encr_prime_bn;
  int encr_param_version;
  int pts;
  int qts;
  int date;
  int seq;
  int binlog_enabled;
  int test_mode;
  int verbosity;
  int unread_messages;
  int active_queries;
  int max_msg_id;
  int started;
  int disable_link_preview;

  long long locks;
  struct tgl_dc *DC_list[TGL_MAX_DC_NUM];
  struct tgl_dc *DC_working;
  int max_dc_num;
  int dc_working_num;
  int enable_pfs;
  int temp_key_expire_time;

  long long cur_uploading_bytes;
  long long cur_uploaded_bytes;
  long long cur_downloading_bytes;
  long long cur_downloaded_bytes;

  char *binlog_name;
  char *auth_file;
  char *downloads_directory;

  struct tgl_update_callback callback;
  struct tgl_net_methods *net_methods;
  void *ev_base;

  char *rsa_key_list[TGL_MAX_RSA_KEYS_NUM];
  // (TGLC_rsa *)
  void *rsa_key_loaded[TGL_MAX_RSA_KEYS_NUM];
  long long rsa_key_fingerprint[TGL_MAX_RSA_KEYS_NUM];
  int rsa_key_num;

  TGLC_bn_ctx *TGLC_bn_ctx;

  struct tgl_allocator *allocator;

  struct tree_peer *peer_tree;
  struct tree_peer_by_name *peer_by_name_tree;
  struct tree_message *message_tree;
  struct tree_message *message_unsent_tree;
  struct tree_photo *photo_tree;
  struct tree_document *document_tree;
  struct tree_webpage *webpage_tree;
  struct tree_encr_document *encr_document_tree;

  int users_allocated;
  int chats_allocated;
  int messages_allocated;
  int channels_allocated;
  int peer_num;
  int peer_size;
  int encr_chats_allocated;
  int geo_chats_allocated;

  tgl_peer_t **Peers;
  struct tgl_message message_list;

  int binlog_fd;

  struct tgl_timer_methods *timer_methods;

  struct tree_query *queries_tree;

  char *base_path;

  struct tree_user *online_updates;

  struct tgl_timer *online_updates_timer;

  int app_id;
  char *app_hash;

  void *ev_login;

  char *app_version;
  int ipv6_enabled;

  struct tree_random_id *random_id_tree;
  struct tree_temp_id *temp_id_tree;

  char *error;
  int error_code;

  int is_bot;

  int last_temp_id;
};
#pragma pack(pop)
//extern struct tgl_state tgl_state;

#ifdef __cplusplus
extern "C" {
#endif

void tgl_reopen_binlog_for_writing (struct tgl_state *TLS);
void tgl_replay_log (struct tgl_state *TLS);

int tgl_print_stat (struct tgl_state *TLS, char *s, int len);
tgl_peer_t *tgl_peer_get (struct tgl_state *TLS, tgl_peer_id_t id);
tgl_peer_t *tgl_peer_get_by_name (struct tgl_state *TLS, const char *s);

struct tgl_message *tgl_message_get (struct tgl_state *TLS, tgl_message_id_t *id);
void tgl_peer_iterator_ex (struct tgl_state *TLS, void (*it)(tgl_peer_t *P, void *extra), void *extra);

int tgl_complete_user_list (struct tgl_state *TLS, int index, const char *text, int len, char **R);
int tgl_complete_chat_list (struct tgl_state *TLS, int index, const char *text, int len, char **R);
int tgl_complete_encr_chat_list (struct tgl_state *TLS, int index, const char *text, int len, char **R);
int tgl_complete_peer_list (struct tgl_state *TLS, int index, const char *text, int len, char **R);
int tgl_complete_channel_list (struct tgl_state *TLS, int index, const char *text, int len, char **R);
int tgl_secret_chat_for_user (struct tgl_state *TLS, tgl_peer_id_t user_id);
int tgl_do_send_bot_auth (struct tgl_state *TLS, const char *code, int code_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *Self), void *callback_extra);

#define TGL_PEER_USER 1
#define TGL_PEER_CHAT 2
#define TGL_PEER_GEO_CHAT 3
#define TGL_PEER_ENCR_CHAT 4
#define TGL_PEER_CHANNEL 5
#define TGL_PEER_TEMP_ID 100
#define TGL_PEER_RANDOM_ID 101
#define TGL_PEER_UNKNOWN 0

#define TGL_MK_USER(id) tgl_set_peer_id (TGL_PEER_USER,id)
#define TGL_MK_CHAT(id) tgl_set_peer_id (TGL_PEER_CHAT,id)
#define TGL_MK_CHANNEL(id) tgl_set_peer_id (TGL_PEER_CHANNEL,id)
#define TGL_MK_GEO_CHAT(id) tgl_set_peer_id (TGL_PEER_GEO_CHAT,id)
#define TGL_MK_ENCR_CHAT(id) tgl_set_peer_id (TGL_PEER_ENCR_CHAT,id)

void tgl_set_binlog_mode (struct tgl_state *TLS, int mode);
void tgl_set_binlog_path (struct tgl_state *TLS, const char *path);
void tgl_set_auth_file_path (struct tgl_state *TLS, const char *path);
void tgl_set_download_directory (struct tgl_state *TLS, const char *path);
void tgl_set_callback (struct tgl_state *TLS, struct tgl_update_callback *cb);
void tgl_set_rsa_key (struct tgl_state *TLS, const char *key);
void tgl_set_rsa_key_direct (struct tgl_state *TLS, unsigned long e, int n_bytes, const unsigned char *n);
void tgl_set_app_version (struct tgl_state *TLS, const char *app_version);

static inline int tgl_get_peer_type (tgl_peer_id_t id) {
  return id.peer_type;
}

static inline int tgl_get_peer_id (tgl_peer_id_t id) {
  return id.peer_id;
}

static inline tgl_peer_id_t tgl_set_peer_id (int type, int id) {
  tgl_peer_id_t ID;
  ID.peer_id = id;
  ID.peer_type = type;
  ID.access_hash = 0;
  return ID;
}

static inline int tgl_cmp_peer_id (tgl_peer_id_t a, tgl_peer_id_t b) {
  return memcmp (&a, &b, 8);
}

void tgl_incr_verbosity (struct tgl_state *TLS);
void tgl_set_verbosity (struct tgl_state *TLS, int val);
void tgl_enable_pfs (struct tgl_state *TLS);
void tgl_set_test_mode (struct tgl_state *TLS);
void tgl_set_net_methods (struct tgl_state *TLS, struct tgl_net_methods *methods);
void tgl_set_timer_methods (struct tgl_state *TLS, struct tgl_timer_methods *methods);
void tgl_set_ev_base (struct tgl_state *TLS, void *ev_base);

int tgl_authorized_dc (struct tgl_state *TLS, struct tgl_dc *DC);
int tgl_signed_dc (struct tgl_state *TLS, struct tgl_dc *DC);

int tgl_init (struct tgl_state *TLS);
void tgl_dc_authorize (struct tgl_state *TLS, struct tgl_dc *DC);

void tgl_dc_iterator (struct tgl_state *TLS, void (*iterator)(struct tgl_dc *DC));
void tgl_dc_iterator_ex (struct tgl_state *TLS, void (*iterator)(struct tgl_dc *DC, void *extra), void *extra);

#define TGL_SEND_MSG_FLAG_DISABLE_PREVIEW 1
#define TGL_SEND_MSG_FLAG_ENABLE_PREVIEW 2

#define TGL_SEND_MSG_FLAG_DOCUMENT_IMAGE TGLDF_IMAGE
#define TGL_SEND_MSG_FLAG_DOCUMENT_STICKER TGLDF_STICKER
#define TGL_SEND_MSG_FLAG_DOCUMENT_ANIMATED TGLDF_ANIMATED
#define TGL_SEND_MSG_FLAG_DOCUMENT_AUDIO TGLDF_AUDIO
#define TGL_SEND_MSG_FLAG_DOCUMENT_VIDEO TGLDF_VIDEO
#define TGL_SEND_MSG_FLAG_DOCUMENT_AUTO 32
#define TGL_SEND_MSG_FLAG_DOCUMENT_PHOTO 64

#define TGL_SEND_MSG_FLAG_REPLY(x) (((unsigned long long)x) << 32)

typedef tgl_peer_id_t tgl_user_id_t;
typedef tgl_peer_id_t tgl_chat_id_t;
typedef tgl_peer_id_t tgl_secret_chat_id_t;
typedef tgl_peer_id_t tgl_user_or_chat_id_t;

void tgl_insert_empty_user (struct tgl_state *TLS, int id);
void tgl_insert_empty_chat (struct tgl_state *TLS, int id);


void tgl_free_all (struct tgl_state *TLS);
void tgl_register_app_id (struct tgl_state *TLS, int app_id, const char *app_hash);

void tgl_login (struct tgl_state *TLS);
void tgl_enable_ipv6 (struct tgl_state *TLS);
void tgl_enable_bot (struct tgl_state *TLS);

struct tgl_state *tgl_state_alloc (void);

void tgl_disable_link_preview (struct tgl_state *TLS);
void tgl_do_lookup_state (struct tgl_state *TLS);

long long tgl_get_allocated_bytes (void);
#ifdef __cplusplus
}
#endif

#endif
