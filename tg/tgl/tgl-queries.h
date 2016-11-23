#ifndef __TGL_QUERIES_H__
#define __TGL_QUERIES_H__

#include "tgl.h"

void tgl_do_get_terms_of_service (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *ans), void *callback_extra);

/* {{{ WORK WITH ACCOUNT */
// sets account password
// user will be requested to type his current password and new password (twice)
void tgl_do_set_password (struct tgl_state *TLS, const char *hint, int hint_len, void (*callback)(struct tgl_state *TLS, void *extra, int success), void *extra);

void tgl_do_set_phone_number (struct tgl_state *TLS, const char *phonenumber, int phonenumber_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);
/* }}} */

/* {{{ SENDING MESSAGES */

struct tl_ds_reply_markup;

// send plain text message to peer id
// flags is combination of TGL_SEND_MSG_FLAG_*
// reply markup can be NULL
void tgl_do_send_message (struct tgl_state *TLS, tgl_peer_id_t peer_id, const char *text, int text_len, unsigned long long flags, struct tl_ds_reply_markup *reply_markup, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// sends plain text reply on message *reply_id*
// message *reply_id* should be cached
void tgl_do_reply_message (struct tgl_state *TLS, tgl_message_id_t *msg_id, const char *text, int text_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// sends contents of text file *file_name* as plain text message
void tgl_do_send_text (struct tgl_state *TLS, tgl_peer_id_t peer_id, const char *file_name, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

void tgl_do_reply_text (struct tgl_state *TLS, tgl_message_id_t *msg_id, const char *file_name, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// send media from file *file_name* to peer *to_id*
// if reply > 0 this message is sent as reply to message *reply*
// *caption* is used only for photos
void tgl_do_send_document (struct tgl_state *TLS, tgl_peer_id_t to_id, const char *file_name, const char *caption, int caption_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);
void tgl_do_reply_document (struct tgl_state *TLS, tgl_message_id_t *msg_id, const char *file_name, const char *caption, int caption_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// forward message *msg_id* to peer *id*
// message can not be encrypted and peer can not be secret chat
void tgl_do_forward_message (struct tgl_state *TLS, tgl_user_or_chat_id_t id, tgl_message_id_t *msg_id, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// forward messages *ids* to peer *id*
// messages can not be encrypted and peer can not be secret chat
void tgl_do_forward_messages (struct tgl_state *TLS, tgl_user_or_chat_id_t id, int size, const tgl_message_id_t *msg_ids[], unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int count, struct tgl_message *ML[]), void *callback_extra);

// sends contact to another user.
// This contact may be or may not be telegram user
void tgl_do_send_contact (struct tgl_state *TLS, tgl_peer_id_t id, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// reply on message *reply_id* with contact
void tgl_do_reply_contact (struct tgl_state *TLS, tgl_message_id_t *reply_id, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// sends media from message *msg_id* to another dialog
// a bit different from forwarding message with media
// secret message media can be forwarded to secret chats
// and non-secret - to non-secret chats and users
void tgl_do_forward_media (struct tgl_state *TLS, tgl_peer_id_t id, tgl_message_id_t *msg_id, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// sends location to chat *id*
void tgl_do_send_location (struct tgl_state *TLS, tgl_peer_id_t id, double latitude, double longitude, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// replies on message *reply_id* with location
void tgl_do_reply_location (struct tgl_state *TLS, tgl_message_id_t *reply_id, double latitude, double longitude, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// sends broadcast (i.e. message to several users at once)
// flags are same as in tgl_do_send_message
void tgl_do_send_broadcast (struct tgl_state *TLS, int num, tgl_user_id_t id[], const char *text, int text_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *extra, int success, int num, struct tgl_message *ML[]), void *callback_extra);

/* }}} */

/* {{{ EDITING SELF PROFILE */
// sets self profile photo
// server will cut central square from this photo
void tgl_do_set_profile_photo (struct tgl_state *TLS, const char *file_name, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// rename self account
void tgl_do_set_profile_name (struct tgl_state *TLS, const char *first_name, int first_name_len, const char *last_name, int last_name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra);

// sets self username
void tgl_do_set_username (struct tgl_state *TLS, const char *username, int username_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra);

// updates online/offline status
void tgl_do_update_status (struct tgl_state *TLS, int online, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// exports card. This card can be later be used by another user to add you to dialog list.
void tgl_do_export_card (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, int *card), void *callback_extra);
/* }}} */

/* {{{ WORKING WITH GROUP CHATS */

// sets chat photo
// server will cut central square from this photo
void tgl_do_set_chat_photo (struct tgl_state *TLS, tgl_chat_id_t chat_id, const char *file_name, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// sets chat title
void tgl_do_rename_chat (struct tgl_state *TLS, tgl_chat_id_t id, const char *new_title, int new_title_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// requests full info about chat *id*.
// if *offline_mode* is set no actual query is sent
void tgl_do_get_chat_info (struct tgl_state *TLS, tgl_chat_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_chat *C), void *callback_extra);

// adds user *id* to chat *chat_id*
// sends *limit* last messages from this chat to user
void tgl_do_add_user_to_chat (struct tgl_state *TLS, tgl_chat_id_t chat_id, tgl_user_id_t id, int limit, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// deleted user *id* from chat *chat_id*
// you can do it if you are admin (=creator) of chat or if you invited this user or if it is yourself
void tgl_do_del_user_from_chat (struct tgl_state *TLS, tgl_chat_id_t chat_id, tgl_user_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// creates group chat with users ids
// there should be at least one user other then you in chat
void tgl_do_create_group_chat (struct tgl_state *TLS, int users_num, tgl_user_id_t ids[], const char *chat_topic, int chat_topic_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// receives invitation link to this chat
// only chat admin can create one
// prevoius link invalidated, if existed
void tgl_do_export_chat_link (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *link), void *callback_extra);

// joins to secret chat by link (or hash of this link)
void tgl_do_import_chat_link (struct tgl_state *TLS, const char *link, int link_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// upgrades chat to channel. 
void tgl_do_upgrade_group (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);
/* }}} */

/* {{{ WORKING WITH USERS */

// requests full info about user *id*.
// if *offline_mode* is set no actual query is sent
void tgl_do_get_user_info (struct tgl_state *TLS, tgl_user_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra);

// adds contact to contact list by phone number
// user will be named  *first_name* *last_name* in contact list
// force should be set to 0
void tgl_do_add_contact (struct tgl_state *TLS, const char *phone, int phone_len, const char *first_name, int first_name_len, const char *last_name, int last_name_len, int force, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *users[]), void *callback_extra);

// deletes user *id* from contact list
void tgl_do_del_contact (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// imports card exported by another user
void tgl_do_import_card (struct tgl_state *TLS, int size, int *card, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_user *U), void *callback_extra);

// blocks user
void tgl_do_block_user (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// unblocks blocked user
void tgl_do_unblock_user (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);
/* }}} */

/* {{{ WORKING WITH SECRET CHATS */

// requests creation of secret chat with user *user_id*
//void tgl_do_create_encr_chat_request (struct tgl_state *TLS, int user_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra);

// accepts secret chat request
// method can fail if another device will be first to accept it
void tgl_do_accept_encr_chat_request (struct tgl_state *TLS, struct tgl_secret_chat *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra);

// sets ttl of secret chat
void tgl_do_set_encr_chat_ttl (struct tgl_state *TLS, struct tgl_secret_chat *E, int ttl, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

// returns secret chat fingerprint
int tgl_do_visualize_key (struct tgl_state *TLS, tgl_secret_chat_id_t id, unsigned char buf[16]);

// requests creation of secret chat with user id
void tgl_do_create_secret_chat (struct tgl_state *TLS, tgl_user_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra);

// cancel a working secret chat
void tgl_do_discard_secret_chat (struct tgl_state *TLS, struct tgl_secret_chat *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra);

/* }}} */

/* {{{ WORKING WITH CHANNELS */
void tgl_do_get_channels_dialog_list (struct tgl_state *TLS, int limit, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra);

void tgl_do_get_channel_info (struct tgl_state *TLS, tgl_peer_id_t id, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_channel *C), void *callback_extra);

void tgl_do_rename_channel (struct tgl_state *TLS, tgl_peer_id_t id, const char *name, int name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_set_channel_photo (struct tgl_state *TLS, tgl_peer_id_t chat_id, const char *file_name, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success), void *callback_extra);

void tgl_do_channel_set_about (struct tgl_state *TLS, tgl_peer_id_t id, const char *about, int about_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_create_channel (struct tgl_state *TLS, int users_num, tgl_peer_id_t ids[], const char *chat_topic, int chat_topic_len, const char *about, int about_len, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_channel_set_username (struct tgl_state *TLS, tgl_peer_id_t id, const char *username, int username_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_join_channel (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_leave_channel (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_channel_invite_user (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_channel_kick_user (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_export_channel_link (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *link), void *callback_extra);

// type:
//   0 - none
//   1 - moderator
//   2 - editor
//   3 - kicked
void tgl_do_channel_set_admin (struct tgl_state *TLS, tgl_peer_id_t channel_id, tgl_peer_id_t user_id, int type, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

void tgl_do_channel_get_members  (struct tgl_state *TLS, tgl_peer_id_t channel_id, int limit, int offset, int type, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *UL[]), void *callback_extra);
/* }}} */

/* {{{ WORKING WITH DIALOG LIST */

// receives all dialogs (except secret chats) from offset=*offset* with limit=*limit*
// dialogs are sorted by last message received
// if limit is > 100 there is a (small) chance of one dialog received twice
void tgl_do_get_dialog_list (struct tgl_state *TLS, int limit, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, tgl_peer_id_t peers[], tgl_message_id_t *last_msg_id[], int unread_count[]), void *callback_extra);

// resolves username
void tgl_do_contact_search (struct tgl_state *TLS, const char *name, int name_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, tgl_peer_t *U), void *callback_extra);

// requests contact list
void tgl_do_update_contact_list (struct tgl_state *TLS, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_user *contacts[]), void *callback_extra);

/* }}} */

/* {{{ WORKING WITH ONE DIALOG */

// marks all inbound messages from peer id as read
void tgl_do_mark_read (struct tgl_state *TLS, tgl_peer_id_t id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// extended mark read
void tgl_do_messages_mark_read (struct tgl_state *TLS, tgl_peer_id_t id, int max_id, int offset, void (*callback)(struct tgl_state *TLS, void *callback_extra, int), void *callback_extra);

// requests last *limit* from offset *offset* (offset = 0 means most recent) messages from dialog with peer id
// if offline_mode=1 then no actual query is sent
// only locally cached messages returned
// also marks messages from this chat as read
void tgl_do_get_history (struct tgl_state *TLS, tgl_peer_id_t id, int offset, int limit, int offline_mode, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra);

// sends typing event to chat
// set status=tgl_typing_typing for default typing event
void tgl_do_send_typing (struct tgl_state *TLS, tgl_peer_id_t id, enum tgl_typing_status status, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

/* }}} */

/* {{{ WORKING WITH MEDIA */

// loads photo/document/document_thumb to downloads directory
// if file is presented it is not redownloaded (but if it is shortened tail will be downloaded)
// returns file name in callback
void tgl_do_load_photo (struct tgl_state *TLS, struct tgl_photo *photo, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_encr_document (struct tgl_state *TLS, struct tgl_encr_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_document (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_video (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_audio (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_audio (struct tgl_state *TLS, struct tgl_document *V, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);
void tgl_do_load_document_thumb (struct tgl_state *TLS, struct tgl_document *video, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);

// loads file by location. Use only for small files!
void tgl_do_load_file_location (struct tgl_state *TLS, struct tgl_file_location *FL, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *file_name), void *callback_extra);

/* }}} */


/* {{{ ANOTHER MESSAGES FUNCTIONS */
// search messages with ids *from* .. *to* in dialog id
// id type of id is UNKNOWN uses global search (in all dialogs) instead
// if *from* or *to* is means *from*=0 and *to*=+INF
// return up to *limit* entries from offset=*offset*
void tgl_do_msg_search (struct tgl_state *TLS, tgl_user_or_chat_id_t id, int from, int to, int limit, int offset, const char *query, int query_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, int size, struct tgl_message *list[]), void *callback_extra);

// deletes message *id*
void tgl_do_delete_msg (struct tgl_state *TLS, tgl_message_id_t *msg_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);

// gets message by *id*
void tgl_do_get_message (struct tgl_state *TLS, tgl_message_id_t *msg_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra);

/* }}} */


/* {{{ EXTENDED QUERIES USE WITH CAUTION */
//#ifndef DISABLE_EXTF
// sends query with extended text syntax
// use only for debug or when you known what are you doing
// since answer is not interpreted by library in any way
void tgl_do_send_extf (struct tgl_state *TLS, const char *data, int data_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, const char *data), void *callback_extra);
int tglf_extf_autocomplete (struct tgl_state *TLS, const char *text, int text_len, int index, char **R, char *data, int data_len);
struct paramed_type *tglf_extf_store (struct tgl_state *TLS, const char *data, int data_len);
char *tglf_extf_fetch (struct tgl_state *TLS, struct paramed_type *T);
//#endif
/* }}} */

/* {{{ BOT */
void tgl_do_start_bot (struct tgl_state *TLS, tgl_peer_id_t bot, tgl_peer_id_t chat, const char *str, int str_len, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success), void *callback_extra);
/* }}} */

#endif
