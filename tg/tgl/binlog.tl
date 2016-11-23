---types---

binlog.encrKey key:64*[int] = binlog.EncrKey;

binlog.peerUser = binlog.PeerType;
binlog.peerChat = binlog.PeerType;
binlog.peerChannel = binlog.PeerType;

binlog.peer peer_type:binlog.PeerType peer_id:int = binlog.Peer;


binlog.start = binlog.Update;

binlog.authKey dc:int key:%binlog.EncrKey = binlog.Update; 
binlog.defaultDc dc:int = binlog.Update;
binlog.dcSigned dc:int = binlog.Update;

binlog.dcOption flags:int dc:int name:string ip:string port:int = binlog.Update;

binlog.ourId id:int = binlog.Update;

binlog.setDhParams root:int prime:%binlog.EncrKey version:int = binlog.Update;



binlog.setPts pts:int = binlog.Update;
binlog.setQts qts:int = binlog.Update;
binlog.setDate date:int = binlog.Update;
binlog.setSeq seq:int = binlog.Update;

binlog.peerDelete peer:%binlog.Peer = binlog.Update;

binlog.encrChat#84977251 flags:# id:int 
  access_hash:flags.17?long 
  date:flags.18?int 
  admin:flags.19?int 
  user_id:flags.20?int
  key:flags.21?%binlog.EncrKey 
  g_key:flags.22?%binlog.EncrKey
  state:flags.23?int 
  ttl:flags.24?int
  layer:flags.25?int
  in_seq_no:flags.26?int last_in_seq_no:flags.26?int out_seq_no:flags.26?int
  key_fingerprint:flags.27?long
    = binlog.Update;

binlog.encrChatExchange#9d49488d flags:# id:int
  exchange_id:flags.17?long
  key:flags.18?%binlog.EncrKey
  state:flags.19?int 
    = binlog.Update;

binlog.user#127cf2f9 flags:# id:int 
  access_hash:flags.17?long 
  first_name:flags.18?string last_name:flags.18?string 
  phone:flags.19?string 
  username:flags.20?string 
  photo:flags.21?Photo 
  real_first_name:flags.22?string real_last_name:flags.22?string 
  user_photo:flags.23?UserProfilePhoto 
  last_read_in:flags.24?int 
  last_read_out:flags.25?int
  bot_info:flags.26?BotInfo
    = binlog.Update;

binlog.chat#0a10aa92 flags:# id:int 
  title:flags.17?string 
  user_num:flags.18?int 
  date:flags.19?int 
  version:flags.20?int participants:flags.20?(Vector ChatParticipant)
  chat_photo:flags.21?ChatPhoto
  photo:flags.22?Photo
  admin:flags.23?int
  last_read_in:flags.24?int 
  last_read_out:flags.25?int
    = binlog.Update;

binlog.channel flags:# id:int
  title:flags.17?string
  username:flags.18?string
  date:flags.19?int
  version:flags.20?int
  chat_photo:flags.21?ChatPhoto
  photo:flags.22?Photo
  about:flags.23?int
  last_read_in:flags.24?int
  admins_count:flags.25?int
  kicked_count:flags.26?int
  access_hash:flags.27?long 
    = binlog.Update;


binlog.chatAddParticipant id:int version:int user_id:int inviter_id:int date:int = binlog.Update;
binlog.chatDelParticipant id:int version:int user_id:int = binlog.Update;

binlog.setMsgId old_id:long new_id:int = binlog.Update;
binlog.messageDelete lid:long = binlog.Update;

binlog.messageNew#427cfcdb flags:# lid:long 
  from:flags.17?%binlog.Peer to:flags.17?%binlog.Peer
  fwd_from_id:flags.18?%binlog.Peer fwd_date:flags.18?int 
  date:flags.19?int 
  message:flags.20?string 
  media:flags.21?MessageMedia 
  action:flags.22?MessageAction 
  reply_id:flags.23?int
  reply_markup:flags.24?ReplyMarkup
    = binlog.Update;

binlog.messageEncrNew#6cf7cabc flags:# lid:long 
  from:flags.17?%binlog.Peer to:flags.17?%binlog.Peer
  //empty 18 bit
  date:flags.19?int 
  message:flags.20?string 
  encr_media:flags.21?DecryptedMessageMedia 
  encr_action:flags.22?DecryptedMessageAction 
  file:flags.23?EncryptedFile 
    = binlog.Update;

binlog.msgUpdate#6dd4d85f lid:long = binlog.Update;

binlog.resetAuthorization = binlog.Update;


---functions---
