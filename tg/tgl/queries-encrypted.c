
/* {{{ Encrypt decrypted */
static int *encr_extra;
static int *encr_ptr;
static int *encr_end;

static char *encrypt_decrypted_message (struct tgl_secret_chat *E) {
  static int msg_key[4];
  static unsigned char sha1a_buffer[20];
  static unsigned char sha1b_buffer[20];
  static unsigned char sha1c_buffer[20];
  static unsigned char sha1d_buffer[20];
  int x = *(encr_ptr);  
  assert (x >= 0 && !(x & 3));
  TGLC_sha1 ((void *)encr_ptr, 4 + x, sha1a_buffer);
  memcpy (msg_key, sha1a_buffer + 4, 16);
 
  static unsigned char buf[64];
  memcpy (buf, msg_key, 16);
  memcpy (buf + 16, E->key, 32);
  TGLC_sha1 (buf, 48, sha1a_buffer);
  
  memcpy (buf, E->key + 8, 16);
  memcpy (buf + 16, msg_key, 16);
  memcpy (buf + 32, E->key + 12, 16);
  TGLC_sha1 (buf, 48, sha1b_buffer);
  
  memcpy (buf, E->key + 16, 32);
  memcpy (buf + 32, msg_key, 16);
  TGLC_sha1 (buf, 48, sha1c_buffer);
  
  memcpy (buf, msg_key, 16);
  memcpy (buf + 16, E->key + 24, 32);
  TGLC_sha1 (buf, 48, sha1d_buffer);

  static unsigned char key[32];
  memcpy (key, sha1a_buffer + 0, 8);
  memcpy (key + 8, sha1b_buffer + 8, 12);
  memcpy (key + 20, sha1c_buffer + 4, 12);

  static unsigned char iv[32];
  memcpy (iv, sha1a_buffer + 8, 12);
  memcpy (iv + 12, sha1b_buffer + 0, 8);
  memcpy (iv + 20, sha1c_buffer + 16, 4);
  memcpy (iv + 24, sha1d_buffer + 0, 8);

  TGLC_aes_key aes_key;
  TGLC_aes_set_encrypt_key (key, 256, &aes_key);
  TGLC_aes_ige_encrypt ((void *)encr_ptr, (void *)encr_ptr, 4 * (encr_end - encr_ptr), &aes_key, iv, 1);
  memset (&aes_key, 0, sizeof (aes_key));

  return (void *)msg_key;
}

static void encr_start (void) {
  encr_extra = packet_ptr;
  packet_ptr += 1; // str len
  packet_ptr += 2; // fingerprint
  packet_ptr += 4; // msg_key
  packet_ptr += 1; // len
}


static void encr_finish (struct tgl_secret_chat *E) {
  int l = packet_ptr - (encr_extra +  8);
  while (((packet_ptr - encr_extra) - 3) & 3) {  
    int t;
    tglt_secure_random (&t, 4);
    out_int (t);
  }

  *encr_extra = ((packet_ptr - encr_extra) - 1) * 4 * 256 + 0xfe;
  encr_extra ++;
  *(long long *)encr_extra = E->key_fingerprint;
  encr_extra += 2;
  encr_extra[4] = l * 4;
  encr_ptr = encr_extra + 4;
  encr_end = packet_ptr;
  memcpy (encr_extra, encrypt_decrypted_message (E), 16);
}
/* }}} */

void tgl_do_send_encr_action (struct tgl_state *TLS, struct tgl_secret_chat *E, struct tl_ds_decrypted_message_action *A) {
  long long t;
  tglt_secure_random (&t, 8);
  int date = time (0);

  struct tgl_message_id id = tgl_peer_id_to_random_msg_id (E->id);
  
  tgl_peer_id_t from_id = TLS->our_id;
  bl_do_edit_message_encr (TLS, &id, &from_id, &E->id, &date, NULL, 0, NULL, A, NULL, TGLMF_PENDING | TGLMF_OUT | TGLMF_UNREAD | TGLMF_CREATE | TGLMF_CREATED | TGLMF_ENCRYPTED);

  struct tgl_message *M = tgl_message_get (TLS, &id);
  assert (M);
  tgl_do_send_msg (TLS, M, 0, 0);
}

void tgl_do_send_encr_chat_layer (struct tgl_state *TLS, struct tgl_secret_chat *E) {
  static struct tl_ds_decrypted_message_action A;
  A.magic = CODE_decrypted_message_action_notify_layer;
  int layer = TGL_ENCRYPTED_LAYER;
  A.layer = &layer;

  tgl_do_send_encr_action (TLS, E, &A);
}

void tgl_do_set_encr_chat_ttl (struct tgl_state *TLS, struct tgl_secret_chat *E, int ttl, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  static struct tl_ds_decrypted_message_action A;
  A.magic = CODE_decrypted_message_action_set_message_t_t_l;
  A.layer = &ttl;

  tgl_do_send_encr_action (TLS, E, &A);
}


/* {{{ Seng msg (plain text, encrypted) */
static int msg_send_encr_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_message *M = q->extra;
  assert (M->flags & TGLMF_ENCRYPTED);
  
  if (M->flags & TGLMF_PENDING) {
    bl_do_edit_message_encr (TLS, &M->permanent_id, NULL, NULL,  
      &M->date,
      NULL, 0, NULL, NULL, NULL, M->flags ^ TGLMF_PENDING);
    
    bl_do_msg_update (TLS, &M->permanent_id);
  }

  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 1, M);
  }
  return 0;
}

static int msg_send_encr_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  struct tgl_message *M = q->extra;
  tgl_peer_t *P = tgl_peer_get (TLS, M->to_id);
  if (P && P->encr_chat.state != sc_deleted && error_code == 400) {
    if (strncmp (error, "ENCRYPTION_DECLINED", 19) == 0) {
      bl_do_peer_delete (TLS, P->id);
    }
  }
  if (q->callback) {
    ((void (*)(struct tgl_state *TLS, void *, int, struct tgl_message *))q->callback) (TLS, q->callback_extra, 0, M);
  }
  if (M) {
    bl_do_message_delete (TLS, &M->permanent_id);
  }
  return 0;
}

static struct query_methods msg_send_encr_methods = {
  .on_answer = msg_send_encr_on_answer,
  .on_error = msg_send_encr_on_error,
  .type = TYPE_TO_PARAM(messages_sent_encrypted_message),
  .name = "send encrypted (message)",
  .timeout = 0,
};
/* }}} */

void tgl_do_send_encr_msg_action (struct tgl_state *TLS, struct tgl_message *M, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  tgl_peer_t *P = tgl_peer_get (TLS, M->to_id);
  if (!P || P->encr_chat.state != sc_ok) { 
    vlogprintf (E_WARNING, "Unknown encrypted chat\n");
    if (callback) {
      callback (TLS, callback_extra, 0, 0);
    }
    return;
  }
 
  assert (M->flags & TGLMF_ENCRYPTED);
  clear_packet ();
  out_int (CODE_messages_send_encrypted_service);
  out_int (CODE_input_encrypted_chat);
  out_int (M->permanent_id.peer_id);
  out_long (M->permanent_id.access_hash);
  out_long (M->permanent_id.id);
  encr_start ();
  out_int (CODE_decrypted_message_layer);
  out_random (15 + 4 * (rand () % 3));
  out_int (TGL_ENCRYPTED_LAYER);
  out_int (2 * P->encr_chat.in_seq_no + (P->encr_chat.admin_id != tgl_get_peer_id (TLS->our_id)));
  out_int (2 * P->encr_chat.out_seq_no + (P->encr_chat.admin_id == tgl_get_peer_id (TLS->our_id)) - 2);
  out_int (CODE_decrypted_message_service);
  out_long (M->permanent_id.id);

  switch (M->action.type) {
  case tgl_message_action_notify_layer:
    out_int (CODE_decrypted_message_action_notify_layer);
    out_int (M->action.layer);
    break;
  case tgl_message_action_set_message_ttl:
    out_int (CODE_decrypted_message_action_set_message_t_t_l);
    out_int (M->action.ttl);
    break;
  case tgl_message_action_request_key:
    out_int (CODE_decrypted_message_action_request_key);
    out_long (M->action.exchange_id);
    out_cstring ((void *)M->action.g_a, 256);
    break;
  case tgl_message_action_accept_key:
    out_int (CODE_decrypted_message_action_accept_key);
    out_long (M->action.exchange_id);
    out_cstring ((void *)M->action.g_a, 256);    
    out_long (M->action.key_fingerprint);
    break;
  case tgl_message_action_commit_key:
    out_int (CODE_decrypted_message_action_commit_key);
    out_long (M->action.exchange_id);
    out_long (M->action.key_fingerprint);
    break;
  case tgl_message_action_abort_key:
    out_int (CODE_decrypted_message_action_abort_key);
    out_long (M->action.exchange_id);
    break;
  case tgl_message_action_noop:
    out_int (CODE_decrypted_message_action_noop);
    break;
  default:
    assert (0);
  }
  encr_finish (&P->encr_chat);
  
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &msg_send_encr_methods, M, callback, callback_extra);
}

void tgl_do_send_encr_msg (struct tgl_state *TLS, struct tgl_message *M, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  if (M->flags & TGLMF_SERVICE) {
    tgl_do_send_encr_msg_action (TLS, M, callback, callback_extra);
    return;
  }
  tgl_peer_t *P = tgl_peer_get (TLS, M->to_id);
  if (!P || P->encr_chat.state != sc_ok) { 
    vlogprintf (E_WARNING, "Unknown encrypted chat\n");
    if (callback) {
      callback (TLS, callback_extra, 0, M);
    }
    return;
  }
  
  assert (M->flags & TGLMF_ENCRYPTED);

  clear_packet ();
  out_int (CODE_messages_send_encrypted);
  out_int (CODE_input_encrypted_chat);
  out_int (tgl_get_peer_id (M->to_id));
  out_long (P->encr_chat.access_hash);
  out_long (M->permanent_id.id);
  encr_start ();
  out_int (CODE_decrypted_message_layer);
  out_random (15 + 4 * (rand () % 3));
  out_int (TGL_ENCRYPTED_LAYER);
  out_int (2 * P->encr_chat.in_seq_no + (P->encr_chat.admin_id != tgl_get_peer_id (TLS->our_id)));
  out_int (2 * P->encr_chat.out_seq_no + (P->encr_chat.admin_id == tgl_get_peer_id (TLS->our_id)) - 2);
  out_int (CODE_decrypted_message);
  out_long (M->permanent_id.id);
  out_int (P->encr_chat.ttl);
  out_cstring ((void *)M->message, M->message_len);
  switch (M->media.type) {
  case tgl_message_media_none:
    out_int (CODE_decrypted_message_media_empty);
    break;
  case tgl_message_media_geo:
    out_int (CODE_decrypted_message_media_geo_point);
    out_double (M->media.geo.latitude);
    out_double (M->media.geo.longitude);
    break;
  default:
    assert (0);
  }
  encr_finish (&P->encr_chat);
  
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &msg_send_encr_methods, M, callback, callback_extra);
}

static int mark_read_encr_on_receive (struct tgl_state *TLS, struct query *q, void *D) {
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int))q->callback)(TLS, q->callback_extra, 1);
  }
  return 0;
}

static int mark_read_encr_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_peer_t *P = q->extra;
  if (P && P->encr_chat.state != sc_deleted && error_code == 400) {
    if (strncmp (error, "ENCRYPTION_DECLINED", 19) == 0) {
      bl_do_peer_delete (TLS, P->id);
    }
  }
  return 0;
}

static struct query_methods mark_read_encr_methods = {
  .on_answer = mark_read_encr_on_receive,
  .on_error = mark_read_encr_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "read encrypted",
  .timeout = 0,
};

void tgl_do_messages_mark_read_encr (struct tgl_state *TLS, tgl_peer_id_t id, long long access_hash, int last_time, void (*callback)(struct tgl_state *TLS, void *callback_extra, int), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_read_encrypted_history);
  out_int (CODE_input_encrypted_chat);
  out_int (tgl_get_peer_id (id));
  out_long (access_hash);
  out_int (last_time);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &mark_read_encr_methods, tgl_peer_get (TLS, id), callback, callback_extra);
}

static int send_encr_file_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_sent_encrypted_message *DS_MSEM = D; 
  struct tgl_message *M = q->extra;

  if (M->flags & TGLMF_PENDING) {
    bl_do_edit_message_encr (TLS, &M->permanent_id, NULL, NULL, DS_MSEM->date, 
    NULL, 0, NULL, NULL, DS_MSEM->file, M->flags ^ TGLMF_PENDING);   
    bl_do_msg_update (TLS, &M->permanent_id);
  }
  
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_message *))q->callback)(TLS, q->callback_extra, 1, M);
  }
  return 0;
}

static struct query_methods send_encr_file_methods = {
  .on_answer = send_encr_file_on_answer,
  .on_error = msg_send_encr_on_error,
  .type = TYPE_TO_PARAM(messages_sent_encrypted_message),
  .name = "send encrypted (file)",
  .timeout = 0,
};

static void send_file_encrypted_end (struct tgl_state *TLS, struct send_file *f, void *callback, void *callback_extra) {
  out_int (CODE_messages_send_encrypted_file);
  out_int (CODE_input_encrypted_chat);
  out_int (tgl_get_peer_id (f->to_id));
  tgl_peer_t *P = tgl_peer_get (TLS, f->to_id);
  assert (P);
  out_long (P->encr_chat.access_hash);
  long long r;
  tglt_secure_random (&r, 8);
  out_long (r);
  encr_start ();
  out_int (CODE_decrypted_message_layer);
  out_random (15 + 4 * (rand () % 3));
  out_int (TGL_ENCRYPTED_LAYER);
  out_int (2 * P->encr_chat.in_seq_no + (P->encr_chat.admin_id != tgl_get_peer_id (TLS->our_id)));
  out_int (2 * P->encr_chat.out_seq_no + (P->encr_chat.admin_id == tgl_get_peer_id (TLS->our_id)));
  out_int (CODE_decrypted_message);
  out_long (r);
  out_int (P->encr_chat.ttl);
  out_string ("");
  int *save_ptr = packet_ptr;
  if (f->flags == -1) {
    out_int (CODE_decrypted_message_media_photo);
  } else if ((f->flags & TGLDF_VIDEO)) {
    out_int (CODE_decrypted_message_media_video);
  } else if ((f->flags & TGLDF_AUDIO)) {
    out_int (CODE_decrypted_message_media_audio);
  } else {
    out_int (CODE_decrypted_message_media_document);
  }
  if (f->flags == -1 || !(f->flags & TGLDF_AUDIO)) {
    out_cstring ("", 0);
    out_int (90);
    out_int (90);
  }
  
  if (f->flags == -1) {
    out_int (f->w);
    out_int (f->h);
  } else if (f->flags & TGLDF_VIDEO) {
    out_int (f->duration);
    out_string (tg_mime_by_filename (f->file_name));
    out_int (f->w);
    out_int (f->h);
  } else if (f->flags & TGLDF_AUDIO) {
    out_int (f->duration);
    out_string (tg_mime_by_filename (f->file_name));
  } else {
    out_string ("");
    out_string (tg_mime_by_filename (f->file_name));
    // document
  }
  
  out_int (f->size);
  out_cstring ((void *)f->key, 32);
  out_cstring ((void *)f->init_iv, 32);
 
  int *save_in_ptr = in_ptr;
  int *save_in_end = in_end;

  in_ptr = save_ptr;
  in_end = packet_ptr;

  assert (skip_type_any (TYPE_TO_PARAM(decrypted_message_media)) >= 0);
  assert (in_ptr == in_end);
  
  in_ptr = save_ptr;
  in_end = packet_ptr;
  
  struct tl_ds_decrypted_message_media *DS_DMM = fetch_ds_type_decrypted_message_media (TYPE_TO_PARAM (decrypted_message_media));
  in_end = save_in_ptr;
  in_ptr = save_in_end;


  int date = time (NULL);


  encr_finish (&P->encr_chat);
  if (f->size < (16 << 20)) {
    out_int (CODE_input_encrypted_file_uploaded);
  } else {
    out_int (CODE_input_encrypted_file_big_uploaded);
  }
  out_long (f->id);
  out_int (f->part_num);
  if (f->size < (16 << 20)) {
    out_string ("");
  }

  unsigned char md5[16];
  unsigned char str[64];
  memcpy (str, f->key, 32);
  memcpy (str + 32, f->init_iv, 32);
  TGLC_md5 (str, 64, md5);
  out_int ((*(int *)md5) ^ (*(int *)(md5 + 4)));

  tfree_secure (f->iv, 32);
 
  tgl_peer_id_t from_id = TLS->our_id;
  
  struct tgl_message_id id = tgl_peer_id_to_msg_id (P->id, r);
  bl_do_edit_message_encr (TLS, &id, &from_id, &f->to_id, &date, NULL, 0, DS_DMM, NULL, NULL, TGLMF_OUT | TGLMF_UNREAD | TGLMF_ENCRYPTED | TGLMF_CREATE | TGLMF_CREATED);

  free_ds_type_decrypted_message_media (DS_DMM, TYPE_TO_PARAM (decrypted_message_media));
  struct tgl_message *M = tgl_message_get (TLS, &id);
  assert (M);
      
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_encr_file_methods, M, callback, callback_extra);
  
  tfree_str (f->file_name);
  tfree (f, sizeof (*f));

}

void tgl_do_send_location_encr (struct tgl_state *TLS, tgl_peer_id_t peer_id, double latitude, double longitude, unsigned long long flags, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_message *M), void *callback_extra) {
  struct tl_ds_decrypted_message_media TDSM;
  TDSM.magic = CODE_decrypted_message_media_geo_point;
  TDSM.latitude = talloc (sizeof (double));
  *TDSM.latitude = latitude;
  TDSM.longitude = talloc (sizeof (double));
  *TDSM.longitude = longitude;
  
  int date = time (0);

  tgl_peer_id_t from_id = TLS->our_id;

  tgl_peer_t *P = tgl_peer_get (TLS, peer_id);
  
  struct tgl_message_id id = tgl_peer_id_to_random_msg_id (P->id);;
  bl_do_edit_message_encr (TLS, &id, &from_id, &peer_id, &date, NULL, 0, &TDSM, NULL, NULL, TGLMF_UNREAD | TGLMF_OUT | TGLMF_PENDING | TGLMF_CREATE | TGLMF_CREATED | TGLMF_ENCRYPTED);

  tfree (TDSM.latitude, sizeof (double));
  tfree (TDSM.longitude, sizeof (double));

  struct tgl_message *M = tgl_message_get (TLS, &id);

  tgl_do_send_encr_msg (TLS, M, callback, callback_extra);
}

/* {{{ Encr accept */
static int send_encr_accept_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_secret_chat *E = tglf_fetch_alloc_encrypted_chat (TLS, D);

  if (E->state == sc_ok) {
    tgl_do_send_encr_chat_layer (TLS, E);
  }
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_secret_chat *))q->callback) (TLS, q->callback_extra, E->state == sc_ok, E);
  }
  return 0;
}

static int send_encr_request_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_secret_chat *E = tglf_fetch_alloc_encrypted_chat (TLS, D);
  
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_secret_chat *))q->callback) (TLS, q->callback_extra, E->state != sc_deleted, E);
  }
  return 0;
}

static int encr_accept_on_error (struct tgl_state *TLS, struct query *q, int error_code, int error_len, const char *error) {
  tgl_peer_t *P = q->extra;
  if (P && P->encr_chat.state != sc_deleted &&  error_code == 400) {
    if (strncmp (error, "ENCRYPTION_DECLINED", 19) == 0) {
      bl_do_peer_delete (TLS, P->id);
    }
  }
  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_secret_chat *))q->callback) (TLS, q->callback_extra, 0, NULL);
  }
  return 0;
}

static struct query_methods send_encr_accept_methods  = {
  .on_answer = send_encr_accept_on_answer,
  .on_error = encr_accept_on_error,
  .type = TYPE_TO_PARAM(encrypted_chat),
  .name = "send encrypted (chat accept)",
  .timeout = 0,
};

static struct query_methods send_encr_request_methods  = {
  .on_answer = send_encr_request_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(encrypted_chat),
  .name = "send encrypted (chat request)",
  .timeout = 0,
};

//int encr_root;
//unsigned char *encr_prime;
//int encr_param_version;
//static TGLC_bn_ctx *ctx;

void tgl_do_send_accept_encr_chat (struct tgl_state *TLS, struct tgl_secret_chat *E, unsigned char *random, void (*callback)(struct tgl_state *TLS,void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  int i;
  int ok = 0;
  for (i = 0; i < 64; i++) {
    if (E->key[i]) {
      ok = 1;
      break;
    }
  }
  if (ok) { 
    if (callback) {
      callback (TLS, callback_extra, 1, E);
    }
    return; 
  } // Already generated key for this chat
  assert (E->g_key);
  assert (TLS->TGLC_bn_ctx);
  unsigned char random_here[256];
  tglt_secure_random (random_here, 256);
  for (i = 0; i < 256; i++) {
    random[i] ^= random_here[i];
  }
  TGLC_bn *b = TGLC_bn_bin2bn (random, 256, 0);
  ensure_ptr (b);
  TGLC_bn *g_a = TGLC_bn_bin2bn (E->g_key, 256, 0);
  ensure_ptr (g_a);
  assert (tglmp_check_g_a (TLS, TLS->encr_prime_bn, g_a) >= 0);
  //if (!ctx) {
  //  ctx = TGLC_bn_ctx_new ();
  //  ensure_ptr (ctx);
  //}
  TGLC_bn *p = TLS->encr_prime_bn;
  TGLC_bn *r = TGLC_bn_new ();
  ensure_ptr (r);
  ensure (TGLC_bn_mod_exp (r, g_a, b, p, TLS->TGLC_bn_ctx));
  static unsigned char kk[256];
  memset (kk, 0, sizeof (kk));
  TGLC_bn_bn2bin (r, kk + (256 - TGLC_bn_num_bytes (r)));
  static unsigned char sha_buffer[20];
  TGLC_sha1 (kk, 256, sha_buffer);

  long long fingerprint = *(long long *)(sha_buffer + 12);

  //bl_do_encr_chat_set_key (TLS, E, kk, *(long long *)(sha_buffer + 12));
  //bl_do_encr_chat_set_sha (TLS, E, sha_buffer);

  int state = sc_ok;

  bl_do_encr_chat (TLS, tgl_get_peer_id (E->id), 
    NULL, NULL, NULL, NULL, 
    kk, NULL, sha_buffer, &state, 
    NULL, NULL, NULL, NULL, NULL, 
    &fingerprint, 
    TGL_FLAGS_UNCHANGED,
    NULL, 0
  );

  clear_packet ();
  out_int (CODE_messages_accept_encryption);
  out_int (CODE_input_encrypted_chat);
  out_int (tgl_get_peer_id (E->id));
  out_long (E->access_hash);
  
  ensure (TGLC_bn_set_word (g_a, TLS->encr_root));
  ensure (TGLC_bn_mod_exp (r, g_a, b, p, TLS->TGLC_bn_ctx));
  static unsigned char buf[256];
  memset (buf, 0, sizeof (buf));
  TGLC_bn_bn2bin (r, buf + (256 - TGLC_bn_num_bytes (r)));
  out_cstring ((void *)buf, 256);

  out_long (E->key_fingerprint);
  TGLC_bn_clear_free (b);
  TGLC_bn_clear_free (g_a);
  TGLC_bn_clear_free (r);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_encr_accept_methods, E, callback, callback_extra);
}

void tgl_do_create_keys_end (struct tgl_state *TLS, struct tgl_secret_chat *U) {
  assert (TLS->encr_prime);
  TGLC_bn *g_b = TGLC_bn_bin2bn (U->g_key, 256, 0);
  ensure_ptr (g_b);
  assert (tglmp_check_g_a (TLS, TLS->encr_prime_bn, g_b) >= 0);
  
  TGLC_bn *p = TLS->encr_prime_bn;
  ensure_ptr (p);
  TGLC_bn *r = TGLC_bn_new ();
  ensure_ptr (r);
  TGLC_bn *a = TGLC_bn_bin2bn ((void *)U->key, 256, 0);
  ensure_ptr (a);
  ensure (TGLC_bn_mod_exp (r, g_b, a, p, TLS->TGLC_bn_ctx));

  unsigned char *t = talloc (256);
  memcpy (t, U->key, 256);
  
  memset (U->key, 0, sizeof (U->key));
  TGLC_bn_bn2bin (r, (void *)(((char *)(U->key)) + (256 - TGLC_bn_num_bytes (r))));
  
  static unsigned char sha_buffer[20];
  TGLC_sha1 ((void *)U->key, 256, sha_buffer);
  long long k = *(long long *)(sha_buffer + 12);
  if (k != U->key_fingerprint) {
    vlogprintf (E_WARNING, "Key fingerprint mismatch (my 0x%" INT64_PRINTF_MODIFIER "x 0x%" INT64_PRINTF_MODIFIER "x)\n", (unsigned long long)k, (unsigned long long)U->key_fingerprint);
    U->state = sc_deleted;
  }

  memcpy (U->first_key_sha, sha_buffer, 20);
  tfree_secure (t, 256);
  
  TGLC_bn_clear_free (g_b);
  TGLC_bn_clear_free (r);
  TGLC_bn_clear_free (a);
}

void tgl_do_send_create_encr_chat (struct tgl_state *TLS, void *x, unsigned char *random, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  int user_id = (long)x;
  int i;
  unsigned char random_here[256];
  tglt_secure_random (random_here, 256);
  for (i = 0; i < 256; i++) {
    random[i] ^= random_here[i];
  }
  TGLC_bn *a = TGLC_bn_bin2bn (random, 256, 0);
  ensure_ptr (a);
  TGLC_bn *p = TGLC_bn_bin2bn (TLS->encr_prime, 256, 0);
  ensure_ptr (p);
 
  TGLC_bn *g = TGLC_bn_new ();
  ensure_ptr (g);

  ensure (TGLC_bn_set_word (g, TLS->encr_root));

  TGLC_bn *r = TGLC_bn_new ();
  ensure_ptr (r);

  ensure (TGLC_bn_mod_exp (r, g, a, p, TLS->TGLC_bn_ctx));

  TGLC_bn_clear_free (a);

  static char g_a[256];
  memset (g_a, 0, 256);

  TGLC_bn_bn2bin (r, (void *)(g_a + (256 - TGLC_bn_num_bytes (r))));
  
  int t = rand ();
  while (tgl_peer_get (TLS, TGL_MK_ENCR_CHAT (t))) {
    t = rand ();
  }

  //bl_do_encr_chat_init (TLS, t, user_id, (void *)random, (void *)g_a);
  
  int state = sc_waiting;
  int our_id = tgl_get_peer_id (TLS->our_id);
  bl_do_encr_chat (TLS, t, NULL, NULL, &our_id, &user_id, random, NULL, NULL, &state, NULL, NULL, NULL, NULL, NULL, NULL, TGLPF_CREATE | TGLPF_CREATED, NULL, 0);

  
  tgl_peer_t *_E = tgl_peer_get (TLS, TGL_MK_ENCR_CHAT (t));
  assert (_E);
  struct tgl_secret_chat *E = &_E->encr_chat;
  
  clear_packet ();
  out_int (CODE_messages_request_encryption);
  tgl_peer_t *U = tgl_peer_get (TLS, TGL_MK_USER (E->user_id));
  assert (U);
  
  out_int (CODE_input_user);
  out_int (E->user_id);
  out_long (U ? U->user.access_hash : 0);

  out_int (tgl_get_peer_id (E->id));
  out_cstring (g_a, 256);
  //write_secret_chat_file ();
  
  TGLC_bn_clear_free (g);
  TGLC_bn_clear_free (p);
  TGLC_bn_clear_free (r);

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_encr_request_methods, E, callback, callback_extra);
}

static int send_encr_discard_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tgl_secret_chat *E = q->extra;

  bl_do_peer_delete (TLS, E->id);

  if (q->callback) {
    ((void (*)(struct tgl_state *, void *, int, struct tgl_secret_chat *))q->callback) (TLS, q->callback_extra, 1, E);
  }
  return 0;
}

static struct query_methods send_encr_discard_methods  = {
  .on_answer = send_encr_discard_on_answer,
  .on_error = q_ptr_on_error,
  .type = TYPE_TO_PARAM(bool),
  .name = "send encrypted (chat discard)",
  .timeout = 0,
};

void tgl_do_discard_secret_chat (struct tgl_state *TLS, struct tgl_secret_chat *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  assert (E);
  assert (tgl_get_peer_id (E->id) > 0);

  if (E->state == sc_deleted || E->state == sc_none) {
    if (callback) {
      callback (TLS, callback_extra, 0, E);
    }
    return;
  }

  clear_packet ();
  out_int (CODE_messages_discard_encryption);
  out_int (tgl_get_peer_id (E->id));

  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &send_encr_discard_methods, E, callback, callback_extra);
}

static int get_dh_config_on_answer (struct tgl_state *TLS, struct query *q, void *D) {
  struct tl_ds_messages_dh_config *DS_MDC = D;

  if (DS_MDC->magic == CODE_messages_dh_config) {
    assert (DS_MDC->p->len == 256);
    bl_do_set_dh_params (TLS, DS_LVAL (DS_MDC->g), (void *)DS_MDC->p->data, DS_LVAL (DS_MDC->version));   
  } else {
    assert (TLS->encr_param_version);
  }
  unsigned char *random = talloc (256);
  assert (DS_MDC->random->len == 256);
  memcpy (random, DS_MDC->random->data, 256);
  
  if (q->extra) {
    void **x = q->extra;
    ((void (*)(struct tgl_state *, void *, void *, void *, void *))(*x))(TLS, x[1], random, q->callback, q->callback_extra);
    tfree (x, 2 * sizeof (void *));
    tfree_secure (random, 256);
  } else {
    tfree_secure (random, 256);
  }
  return 0;
}

static struct query_methods get_dh_config_methods  = {
  .on_answer = get_dh_config_on_answer,
  .on_error = q_void_on_error,
  .type = TYPE_TO_PARAM(messages_dh_config),
  .name = "dh config",
  .timeout = 0,
};

void tgl_do_accept_encr_chat_request (struct tgl_state *TLS, struct tgl_secret_chat *E, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  if (E->state != sc_request) {
    if (callback) {
      callback (TLS, callback_extra, 0, E);
    }
    return;
  }
  assert (E->state == sc_request);
  
  clear_packet ();
  out_int (CODE_messages_get_dh_config);
  out_int (TLS->encr_param_version);
  out_int (256);
  void **x = talloc (2 * sizeof (void *));
  x[0] = tgl_do_send_accept_encr_chat;
  x[1] = E;
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_dh_config_methods, x, callback, callback_extra);
}

void tgl_do_create_encr_chat_request (struct tgl_state *TLS, int user_id, void (*callback)(struct tgl_state *TLS, void *callback_extra, int success, struct tgl_secret_chat *E), void *callback_extra) {
  clear_packet ();
  out_int (CODE_messages_get_dh_config);
  out_int (TLS->encr_param_version);
  out_int (256);
  void **x = talloc (2 * sizeof (void *));
  x[0] = tgl_do_send_create_encr_chat;
  x[1] = (void *)(long)(user_id);
  tglq_send_query (TLS, TLS->DC_working, packet_ptr - packet_buffer, packet_buffer, &get_dh_config_methods, x, callback, callback_extra);
}
/* }}} */
