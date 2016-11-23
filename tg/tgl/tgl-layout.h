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
#ifndef __TGL_LAYOUT_H__
#define __TGL_LAYOUT_H__

#define TGLDF_IMAGE 1
#define TGLDF_STICKER 2
#define TGLDF_ANIMATED 4
#define TGLDF_AUDIO 8
#define TGLDF_VIDEO 16

#define TGLMF_UNREAD 1
#define TGLMF_OUT 2
#define TGLMF_DISABLE_PREVIEW 4
#define TGLMF_MENTION 16
#define TGLMF_CREATED (1 << 8)
#define TGLMF_PENDING (1 << 9)
#define TGLMF_DELETED (1 << 10)
#define TGLMF_ENCRYPTED (1 << 11)
#define TGLMF_EMPTY (1 << 12)
#define TGLMF_SERVICE (1 << 13)
#define TGLMF_SESSION_OUTBOUND (1 << 14)
#define TGLMF_POST_AS_CHANNEL (1 << 8)
#define TGLMF_HTML (1 << 9)

#define TGLMF_CREATE 0x10000


#define TGLPF_CREATED (1 << 0)
#define TGLPF_CREATE 0x80000000
#define TGLPF_HAS_PHOTO (1 << 1)
#define TGLPF_DELETED (1 << 2)
#define TGLPF_OFFICIAL (1 << 3)
#define TGLPF_KICKED (1 << 4)
#define TGLPF_ADMIN (1 << 5)
#define TGLPF_CREATOR (1 << 6)
#define TGLPF_LEFT (1 << 7)
#define TGLPF_DEACTIVATED (1 << 8)

#define TGLUF_CONTACT (1 << 16)
#define TGLUF_MUTUAL_CONTACT (1 << 17)
#define TGLUF_BLOCKED (1 << 18)
#define TGLUF_SELF (1 << 19)
#define TGLUF_CREATED TGLPF_CREATED
#define TGLUF_DELETED TGLPF_DELETED
#define TGLUF_HAS_PHOTO TGLPF_HAS_PHOTO
#define TGLUF_CREATE TGLPF_CREATE
#define TGLUF_BOT (1 << 20)
#define TGLUF_OFFICIAL TGLPF_OFFICIAL

#define TGLUF_TYPE_MASK \
  (TGLUF_CONTACT | TGLUF_MUTUAL_CONTACT | TGLUF_BLOCKED | TGLUF_SELF | TGLUF_CREATED | TGLUF_DELETED | TGLUF_OFFICIAL)

#define TGLCF_CREATED TGLPF_CREATED
#define TGLCF_CREATE TGLPF_CREATE
#define TGLCF_HAS_PHOTO TGLPF_HAS_PHOTO
#define TGLCF_KICKED TGLPF_KICKED
#define TGLCF_CREATOR TGLPF_CREATOR
#define TGLCF_ADMIN TGLPF_ADMIN
#define TGLCF_OFFICIAL TGLPF_OFFICIAL
#define TGLCF_LEFT TGLPF_LEFT
#define TGLCF_DEACTIVATED TGLPF_DEACTIVATED
#define TGLCF_ADMINS_ENABLED (1 << 16)

#define TGLCF_TYPE_MASK \
  (TGLCF_CREATED | TGLCF_KICKED | TGLCF_CREATOR | TGLCF_ADMIN | TGLCF_OFFICIAL | TGLCF_LEFT | TGLCF_DEACTIVATED | TGLCF_ADMINS_ENABLED)

#define TGLECF_CREATED TGLPF_CREATED
#define TGLECF_CREATE TGLPF_CREATE
#define TGLECF_HAS_PHOTO TGLPF_HAS_PHOTO
#define TGLECF_KICKED TGLPF_KICKED
#define TGLECF_CREATOR TGLPF_CREATOR
#define TGLECF_ADMIN TGLPF_ADMIN

#define TGLECF_TYPE_MASK \
  (TGLECF_CREATED | TGLECF_KICKED | TGLECF_CREATOR | TGLECF_ADMIN)

#define TGLCHF_CREATED TGLPF_CREATED
#define TGLCHF_CREATE TGLPF_CREATE
#define TGLCHF_HAS_PHOTO TGLPF_HAS_PHOTO
#define TGLCHF_KICKED TGLPF_KICKED
#define TGLCHF_CREATOR TGLPF_CREATOR
#define TGLCHF_ADMIN TGLPF_ADMIN
#define TGLCHF_OFFICIAL TGLPF_OFFICIAL
#define TGLCHF_LEFT TGLPF_LEFT
#define TGLCHF_DEACTIVATED TGLPF_DEACTIVATED
#define TGLCHF_BROADCAST (1 << 16)
#define TGLCHF_EDITOR (1 << 17)
#define TGLCHF_MODERATOR (1 << 18)
#define TGLCHF_MEGAGROUP (1 << 19)

#define TGLCHF_TYPE_MASK \
  (TGLCHF_CREATED | TGLCHF_KICKED | TGLCHF_CREATOR | TGLCHF_ADMIN | TGLCHF_OFFICIAL | TGLCHF_LEFT | TGLCHF_DEACTIVATED | TGLCHF_BROADCAST | TGLCHF_EDITOR | TGLCHF_MODERATOR | TGLCHF_MEGAGROUP)


#define TGLCHF_DIFF 0x20000000

#define TGL_FLAGS_UNCHANGED 0x40000000

#define TGLDCF_AUTHORIZED 1
#define TGLDCF_LOGGED_IN 8

#define TGL_PERMANENT_ID_SIZE 24
#pragma pack(push,4)

typedef struct { 
  int peer_type; 
  int peer_id; 
  long long access_hash;
} tgl_peer_id_t;

enum tgl_dc_state {
  st_init,
  st_reqpq_sent,
  st_reqdh_sent,
  st_client_dh_sent,
  st_init_temp,
  st_reqpq_sent_temp,
  st_reqdh_sent_temp,
  st_client_dh_sent_temp,
  st_authorized,
  st_error
};

#define MAX_DC_SESSIONS 3

struct tgl_session {
  struct tgl_dc *dc;
  long long session_id;
  long long last_msg_id;
  int seq_no;
  int received_messages;
  struct connection *c;
  struct tree_long *ack_tree;
  struct tgl_timer *ev;
};

struct tgl_dc_option {
  struct tgl_dc_option *next;
  char *ip;
  int port;
};

struct tgl_dc {
  int id;
  //int port;
  int flags;
  int rsa_key_idx;
  enum tgl_dc_state state;
  //char *ip;
  //char *user;
  struct tgl_session *sessions[MAX_DC_SESSIONS];
  char auth_key[256];
  char temp_auth_key[256];
  char nonce[256];
  char new_nonce[256];
  char server_nonce[256];
  long long auth_key_id;
  long long temp_auth_key_id;
  long long temp_auth_key_bind_query_id;

  long long server_salt;
  struct tgl_timer *ev;

  int server_time_delta;
  double server_time_udelta;

  // ipv4, ipv6, ipv4_media, ipv6_media
  struct tgl_dc_option *options[4];
};

enum tgl_message_entity_type {
tgl_message_entity_unknown,
tgl_message_entity_mention,
tgl_message_entity_hashtag,
tgl_message_entity_bot_command,
tgl_message_entity_url,
tgl_message_entity_email,
tgl_message_entity_bold,
tgl_message_entity_italic,
tgl_message_entity_code,
tgl_message_entity_pre,
tgl_message_entity_text_url
};

struct tgl_message_entity {
  enum tgl_message_entity_type type;
  int start;
  int length;
  char *extra;
};

enum tgl_message_media_type {
  tgl_message_media_none,
  tgl_message_media_photo,
  tgl_message_media_document,
  tgl_message_media_geo,
  tgl_message_media_contact,
  tgl_message_media_unsupported,
  //tgl_message_media_photo_encr,
  //tgl_message_media_video_encr,
  //tgl_message_media_audio_encr,
  tgl_message_media_document_encr,
  tgl_message_media_webpage,
  tgl_message_media_venue,
  tgl_message_media_video,
  tgl_message_media_audio
};

enum tgl_message_action_type {
  tgl_message_action_none,
  tgl_message_action_geo_chat_create,
  tgl_message_action_geo_chat_checkin,
  tgl_message_action_chat_create,
  tgl_message_action_chat_edit_title,
  tgl_message_action_chat_edit_photo,
  tgl_message_action_chat_delete_photo,
  tgl_message_action_chat_add_users,
  tgl_message_action_chat_add_user_by_link,
  tgl_message_action_chat_delete_user,
  tgl_message_action_set_message_ttl,
  tgl_message_action_read_messages,
  tgl_message_action_delete_messages,
  tgl_message_action_screenshot_messages,
  tgl_message_action_flush_history,
  tgl_message_action_resend,
  tgl_message_action_notify_layer,
  tgl_message_action_typing,
  tgl_message_action_noop,
  tgl_message_action_commit_key,
  tgl_message_action_abort_key,
  tgl_message_action_request_key,
  tgl_message_action_accept_key,
  tgl_message_action_channel_create,
  tgl_message_action_migrated_to,
  tgl_message_action_migrated_from
};

enum tgl_typing_status {
  tgl_typing_none,
  tgl_typing_typing,
  tgl_typing_cancel,
  tgl_typing_record_video,
  tgl_typing_upload_video,
  tgl_typing_record_audio,
  tgl_typing_upload_audio,
  tgl_typing_upload_photo,
  tgl_typing_upload_document,
  tgl_typing_geo,
  tgl_typing_choose_contact
};

struct tgl_file_location {
  int dc;
  long long volume;
  int local_id;
  long long secret;
};

struct tgl_photo_size {
  char *type;
  struct tgl_file_location loc;
  int w;
  int h;
  int size;
  char *data;
};

struct tgl_geo {
  double longitude;
  double latitude;
};

struct tgl_photo {
  long long id;
  long long access_hash;
  int refcnt;
  //int user_id;
  int date;
  char *caption;
  //struct tgl_geo geo;
  int sizes_num;
  struct tgl_photo_size *sizes;
};

struct tgl_encr_document {
  long long id;
  long long access_hash;
  int refcnt;

  int dc_id;
  int size;
  int key_fingerprint;
  int flags;
  
  unsigned char *key;
  unsigned char *iv;
  int w;
  int h;
  char *caption;
  char *mime_type;
  int duration;
};


struct tgl_user_status {
  int online;
  int when;
  struct tgl_timer *ev;
};

struct tgl_bot_command {
  char *command;
  char *description;
};

struct tgl_bot_info {
  int version;
  char *share_text;
  char *description;
  int commands_num;
  struct tgl_bot_command *commands;
};

struct tgl_user {
  tgl_peer_id_t id;
  int flags;
  struct tgl_message *last;
  char *print_name;
  char *username;
  int structure_version;
  struct tgl_file_location photo_big;
  struct tgl_file_location photo_small;
  int last_read_in;
  int last_read_out;
  long long photo_id;
  struct tgl_photo *photo;
  void *extra;
  char *first_name;
  char *last_name;
  char *phone;
  long long access_hash;
  struct tgl_user_status status;
  int blocked;
  char *real_first_name;
  char *real_last_name;

  struct tgl_bot_info *bot_info;
};

struct tgl_channel {
  tgl_peer_id_t id;
  int flags;
  struct tgl_message *last;
  char *print_title;
  char *username;
  int structure_version;
  struct tgl_file_location photo_big;
  struct tgl_file_location photo_small;
  int last_read_in;
  int last_read_out;
  long long photo_id;
  struct tgl_photo *photo;
  void *extra;

  long long access_hash;
  int date;
  char *title;
  int version;
  char *about;
  int participants_count;
  int admins_count;
  int kicked_count;

  int pts;
};

struct tgl_chat_user {
  int user_id;
  int inviter_id;
  int date;
};

struct tgl_chat {
  tgl_peer_id_t id;
  int flags;
  struct tgl_message *last;
  char *print_title;
  char *username;
  int structure_version;
  struct tgl_file_location photo_big;
  struct tgl_file_location photo_small;
  int last_read_in;
  int last_read_out;
  long long pad;
  struct tgl_photo *photo;
  void *extra;
  char *title;
  int users_num;
  int user_list_size;
  int user_list_version;
  struct tgl_chat_user *user_list;
  int date;
  int version;
  int admin_id;
};

enum tgl_secret_chat_state {
  sc_none,
  sc_waiting,
  sc_request,
  sc_ok,
  sc_deleted
};

enum tgl_secret_chat_exchange_state {
  tgl_sce_none,
  tgl_sce_requested,
  tgl_sce_accepted,
  tgl_sce_committed,
  tgl_sce_confirmed,
  tgl_sce_aborted
};

struct tgl_secret_chat {
  tgl_peer_id_t id;
  int flags;
  struct tgl_message *last;
  char *print_name;
  char *username;
  int structure_version;
  struct tgl_file_location photo_big;
  struct tgl_file_location photo_small;
  int pad1;
  int pad2;
  long long pad;
  struct tgl_photo *photo;
  void *extra;
  int user_id;
  int admin_id;
  int date;
  int ttl;
  int layer;
  int in_seq_no;
  int out_seq_no;
  int last_in_seq_no;
  long long access_hash;
  unsigned char *g_key;

  enum tgl_secret_chat_state state;
  int key[64];
  long long key_fingerprint;
  unsigned char first_key_sha[20];

  long long exchange_id;
  enum tgl_secret_chat_exchange_state exchange_state;
  int exchange_key[64];
  long long exchange_key_fingerprint;
};

typedef union tgl_peer {
  struct {
    tgl_peer_id_t id;
    int flags;
    struct tgl_message *last;
    char *print_name;
    char *username;
    int structure_version;
    struct tgl_file_location photo_big;
    struct tgl_file_location photo_small;
    struct tgl_photo *photo;
    int last_read_in;
    int last_read_out;
    long long photo_id;
    void *extra;
  };
  struct tgl_user user;
  struct tgl_chat chat;
  struct tgl_channel channel;
  struct tgl_secret_chat encr_chat;
} tgl_peer_t;

struct tgl_document {
  long long id;
  long long access_hash;
  int refcnt;
  //int user_id;
  int date;
  int size;
  int dc_id;
  struct tgl_photo_size thumb;
  char *caption;
  char *mime_type;

  int w;
  int h;
  int flags;
  int duration;
};

struct tgl_message_action {
  enum tgl_message_action_type type;
  union {
    struct {
      char *title;
      int user_num;
      int *users;
    };
    char *new_title;
    struct tgl_photo *photo;
    int user;
    int ttl;
    int layer;
    int read_cnt;
    int delete_cnt;
    int screenshot_cnt;
    enum tgl_typing_status typing;
    struct {
      int start_seq_no;
      int end_seq_no;
    };
    struct {
      unsigned char *g_a;
      long long exchange_id;
      long long key_fingerprint;
    };
  };
};


struct tgl_webpage {
  long long id;
  int refcnt;
  char *url;
  char *display_url;
  char *type;
  char *site_name;
  char *title;
  char *description;
  struct tgl_photo *photo;
  char *embed_url;
  char *embed_type;
  int embed_width;
  int embed_height;
  int duration;
  char *author;
};

struct tgl_message_media {
  enum tgl_message_media_type type;
  union {
    struct {
      union {
        struct tgl_photo *photo;
        struct tgl_document *document;
      };
      char *caption;
    };
    
    struct tgl_encr_document *encr_document;
    struct tgl_webpage *webpage;

    struct tgl_geo geo;
    struct {
      char *phone;
      char *first_name;
      char *last_name;
      int user_id;
    };
    
    struct {
      void *data;
      int data_size;
    };
    struct {
      struct tgl_geo geo;
      char *title;
      char *address;
      char *provider;
      char *venue_id;
    } venue;
  };
};

struct tgl_message_reply_markup {
  int refcnt;
  int flags;
  int rows;
  int *row_start;
  char **buttons;
};

typedef struct tgl_message_id {
  unsigned peer_type;
  unsigned peer_id;
  long long id;
  long long access_hash;
} tgl_message_id_t;

struct tgl_message {
  struct tgl_message *next_use, *prev_use;
  struct tgl_message *next, *prev;
  int temp_id;
  long long server_id;
  long long random_id;
  struct tgl_message_id permanent_id;
  int flags;
  tgl_peer_id_t fwd_from_id;
  int fwd_date;
  int reply_id;
  tgl_peer_id_t from_id;
  tgl_peer_id_t to_id;
  int date;
  struct tgl_message_reply_markup *reply_markup;
  int entities_num;
  struct tgl_message_entity *entities;
  union {
    struct tgl_message_action action;
    struct {
      char *message;
      int message_len;
      struct tgl_message_media media;
    };
  };
};
#pragma pack(pop)
#endif
