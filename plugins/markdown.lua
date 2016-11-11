local command = 'markdown'
local doc = [[```
/bold text |and reply
/italic text |and reply
/code test |and reply
--------------------
/boldch @UsernameChannel text
/italicch @UsernameChannel text
/codech @UsernameChannel text
/linkch @UsernameChannel text url
--------------------
inline type @NicRobot text
```]]
local triggers = {
  '^[!/#]bold[@'..bot.username..']*',
  '^[!/#]italic[@'..bot.username..']*',
  '^[!/#]code[@'..bot.username..']*',
  '^[!/#]link[@'..bot.username..']*',
  --send to channel
  '^[!/#]boldch[@'..bot.username..']*',
  '^[!/#]italicch[@'..bot.username..']*',
  '^[!/#]codech[@'..bot.username..']*',
  '^[!/#]linkch[@'..bot.username..']*',
  --inline Markdown
  '^/!/inline'
}

local action = function(msg)
sendChatAction(msg.chat.id, 'typing')

    if msg.text:match("^[!/#]bold") and msg.reply_to_message then
        sendMessage(msg.chat.id, '*'..msg.reply_to_message.text..'*',true,nil,true)
   return
end
   if msg.text:match("^[!/#]italic") and msg.reply_to_message then
        sendMessage(msg.chat.id, '_'..msg.reply_to_message.text..'_',true,nil,true)
   return
end
   if msg.text:match("^[!/#]code") and msg.reply_to_message then
        sendMessage(msg.chat.id, '`'..msg.reply_to_message.text..'`',true,nil,true)
   return
end
   if msg.text:match("^[!/#]link ") and msg.reply_to_message then
        local matches = { string.match(msg.text, "^[!/#]link (.*)") }
        sendMessage(msg.chat.id, '['..msg.reply_to_message.text..']('..matches[1]..')',true,nil,true)
   return
end

   if msg.text_lower:match('^[!/#]bold (.*)') then
        local matches = { string.match(msg.text, "^[!/#]bold (.*)") }
    	local text = '*'..matches[1]..'*'
        sendMessage(msg.chat.id, text, true, false, true)
    return
end
   if msg.text_lower:match('^[!/#]italic (.*)') then
        local matches = { string.match(msg.text, "^[!/#]italic (.*)") }
    	local text = '_'..matches[1]..'_'
        sendMessage(msg.chat.id, text, true, false, true)
    return
end
   if msg.text_lower:match('^[!/#]code (.*)') then
        local matches = { string.match(msg.text, "^[!/#]code (.*)") }
    	local text = '`'..matches[1]..'`'
        sendMessage(msg.chat.id, text, true, false, true)
    return
end
   if msg.text:match("^[!/#]link (.*) (.*)") then
        local matches = { string.match(msg.text, "^[!/#]link (.*) (.*)") }
        local text = '['..matches[2]..']('..matches[1]..')'
        sendMessage(msg.chat.id, text, true, false, true)
    return
end 
--send your pm to channel
	if msg.text_lower:match('^[!/#]boldch (.*) (.*)') then
	    local matches = { string.match(msg.text, "^[!/#]boldch (.*) (.*)") }
	    local text = '*'..matches[2]..'*'
     	local channel = matches[1]
	    sendMessage(channel, text, true, false, true) 
		sendReply(msg, 'I was Sent [ '..text..' ] To '..channel, true,nil,true)

    return
end
	if msg.text_lower:match('^[!/#]italicch (.*) (.*)') then
	    local matches = { string.match(msg.text, "^[!/#]italicch (.*) (.*)") }
	    local text = '_'..matches[2]..'_'
     	local channel = matches[1]
	    sendMessage(channel, text, true, false, true) 
		sendReply(msg, 'I was Sent [ '..text..' ] To '..channel, true,nil,true)

    return
end
	if msg.text_lower:match('^[!/#]codech (.*) (.*)') then
	    local matches = { string.match(msg.text, "^[!/#]codech (.*) (.*)") }
	    local text = '`'..matches[2]..'`'
     	local channel = matches[1]
	    sendMessage(channel, text, true, false, true) 
		sendReply(msg, 'I was Sent [ '..text..' ] To '..channel, true,nil,true)

    return
end
    if msg.text:match("^/linkch (.*) (.*) (.*)") then
         local matches = { string.match(msg.text, "^/linkch (.*) (.*) (.*)") }
         local text = '['..matches[2]..']('..matches[3]..')'
         local channel = matches[1]
         sendMessage(channel, text, true, false, true)
		 sendReply(msg, 'I was Sent '..text..' To '..channel, true,nil,true)
    return     
end
    local input =URL.escape(msg.text:input())
    local text = "["..inline_block('Bold', '*'..input..'*','*'..input..'*','http://s6.picofile.com/file/8247733176/B.png')..','
    text = text .. inline_block('Italic', '_'..input..'_','_'..input..'_','http://s7.picofile.com/file/8247733234/I.png')..', '
    text = text .. inline_block('Code', ''..input..'',''..input..'','http://s7.picofile.com/file/8247733776/C2.png')..', '
    text = text .. inline_block('Custome', input,input,'http://s6.picofile.com/file/8247733200/custom.png')..']'
    sendInline(msg.id, text)
end

return {
  action = action,
  triggers = triggers,
  doc = doc,
  command = command
}
