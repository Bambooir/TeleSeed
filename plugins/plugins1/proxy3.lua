local aibot="mokubot" local pendingUsers={} local function 
pre_process(msg)
  -- Ignore service msg
  if msg.service then
    return msg
  end
  if msg.from.username == aibot and next(pendingUsers) ~= nil then
    vardump(msg.from.username)
    send_msg(pendingUsers[#pendingUsers], msg.text , ok_cb, false)
    table.remove(pendingUsers,#pendingUsers)
  end
  return msg end local function callbackres(extra, success, result)
	-- 1=msg 2=text
	local msg
	--vardump(extra[1])
	if result ~= false then
		msg= result.peer_type .. "#id" .. result.peer_id
		send_msg(msg, extra[2] , ok_cb, false)
		table.insert(pendingUsers,get_receiver(extra[1]))
	
	else
		msg="username lookup failed"
		send_msg(get_receiver(extra[1]), msg , ok_cb, false)
	end end local function run(msg, matches)
   
   resolve_username(aibot, callbackres,{msg, matches[1]})
   
   return text end return {
  description = "sends and receives from another user/bot",
  usage = "!p [whatever]",
  patterns = {
     "^[/!#]otouto +(.+)$"
  },
  run = run,
  pre_process = pre_process
}
