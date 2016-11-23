local command = 'voice' 
local doc = [[```
/tts [text]
```]]
local triggers = {
  "^[!/#]tts",
}

download_file = function(url, filename)

  local respbody = {}
  local options = {
    url = url,
    sink = ltn12.sink.table(respbody),
    redirect = true
  }

  local response = nil

  if url:match('^https') then
    options.redirect = false
    response = { HTTPS.request(options) }
  else
    response = { HTTP.request(options) }
  end

  local code = response[2]
  local headers = response[3]
  local status = response[4]

  if code ~= 200 then return false end

  filename = filename or './' .. os.time()

  local file = io.open(filename, 'w+')
  file:write(table.concat(respbody))
  file:close()

  return filename

end
local texttovoice = function(msg)
    sendChatAction(msg.chat.id, 'record_audio ')
  local matches = { string.match(msg.text, "^[!/#]tts (.*)") }
  local urlvc = "http://tts.baidu.com/text2audio?lan=en&ie=UTF-8&text="..matches[1]
  local file = download_file(urlvc,'vc.mp3')
  sendVoice(msg.chat.id,'vc.mp3')
end

local action = function(msg)
if msg.text:match("^[!/#]tts (.*)") then
return texttovoice(msg)
end
end
return {
	action = action,
	triggers = triggers,
	doc = doc,
	command = command
}
