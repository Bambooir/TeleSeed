---types---
decryptedMessageMediaVideoL12#4cee6ef3 str_thumb:bytes thumb_w:int thumb_h:int duration:int w:int h:int size:int key:bytes iv:bytes = DecryptedMessageMedia;
decryptedMessageMediaAudioL12#6080758f duration:int size:int key:bytes iv:bytes = DecryptedMessageMedia;

updateMsgUpdate id:int pts:int pts_count:int = Update;

messageMediaPhotoL27#c8c45a2a photo:Photo = MessageMedia;
messageMediaVideoL27#a2d24290 video:Video = MessageMedia;
//messageMediaDocumentL27#2fda2204 document:Document = MessageMedia;
//messageMediaAudioL27#c6b68300 audio:Audio = MessageMedia;
---functions---
