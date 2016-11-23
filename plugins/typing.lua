local function reallySendTyping(extra) 
	send_typing (extra[1], ok_cb, false)
	print(extra[1])
	end

local function sendtyping(extra,success,result)
	cid = false
	print(result,success)
	if (result ~= false and success)  then
		cid=  result.peer_type .. "#id" .. result.peer_id 
	elseif (result == false and success) then
		cid = extra[1]
	end

	if cid ~= false then
		elapsed = 0
		while (elapsed < tonumber(extra[2])) do
			postpone (reallySendTyping, {cid}, elapsed)
			elapsed = elapsed + 5			
		end
	else
		send_msg(get_receiver(extra[1]), "username lookup failed", ok_cb, false)	
	end
end

local function run(msg, matches)
	
	i=math.random(0,9)
	-- send_typing(msg.from.id)
	
	if (string.find(matches[2], '#')) then
		sendtyping({matches[2], matches[1]}, 1, false)
	else
		resolve_username(matches[2],  sendtyping,{msg, matches[1]})
	end
  -- return text
 	 return false
end

return {
  description = "Type forever",
  usage = "!typing [secs] @[username] or id",
  patterns = {
     "^[/!#]typing ([0-9]+) @(%w+)$",
     "^[/!#]typing ([0-9]+) (.+)$"
  }, 
  run = run 
}
