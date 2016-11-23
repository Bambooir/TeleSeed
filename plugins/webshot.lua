local command = 'webshot'
local doc = [[```
/webshot http://<website.com>
Generate a webshot from a website
```]]

local triggers = {
	"^[!/#]webshot"
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
local action = function(msg)
    	sendChatAction(msg.chat.id, 'upload_photo')
local input = msg.text:input()
local url = "http://api.screenshotmachine.com/?key=b645b8&size=X&url="..input
  local file = download_file(url,'webshot.jpg')
      sendPhoto(msg.chat.id,file,input)
end
return {
	action = action,
	triggers = triggers,
	doc = doc,
	command = command
}
