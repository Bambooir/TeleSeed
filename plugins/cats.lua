if not config.thecatapi_key then
	print('Missing config value: thecatapi_key.')
	print('cats.lua will be enabled, but there are more features with a key.')
end

local command = 'cat'
local doc = '`Returns a cat!`'

local triggers = {
	'^[!/#]cat[@'..bot.username..']*$'
}

local action = function(msg)
sendChatAction(msg.chat.id, 'upload_photo')
	local url = 'http://thecatapi.com/api/images/get?format=html&type=jpg'
	if config.thecatapi_key then
		url = url .. '&api_key=' .. config.thecatapi_key
	end

	local str, res = HTTP.request(url)
	if res ~= 200 then
		sendReply(msg, config.errors.connection)
		return
	end

	str = str:match('<img src="(.-)">')
	local output = '[Cat!]('..str..')'

	sendMessage(msg.chat.id, output, false, nil, true)

end

return {
	action = action,
	triggers = triggers,
	doc = doc,
	command = command
}
