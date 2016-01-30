local function set_bot_photo(msg, success, result)
  local receiver = get_receiver(msg)
  if success then
    local file = 'data/photos/bot.jpg'
    print('File downloaded to:', result)
    os.rename(result, file)
    print('File moved to:', file)
    set_profile_photo(file, ok_cb, false)
    send_large_msg(receiver, 'Photo changed!', ok_cb, false)
    redis:del("bot:photo")
  else
    print('Error downloading: '..msg.id)
    send_large_msg(receiver, 'Failed, please try again!', ok_cb, false)
  end
end
local function parsed_url(link)
  local parsed_link = URL.parse(link)
  local parsed_path = URL.parse_path(parsed_link.path)
  return parsed_path[2]
end

local function run(msg,matches)
    local data = load_data(_config.moderation.data)
    local receiver = get_receiver(msg)
    local group = msg.to.id
    if not is_admin(msg) then
    	return
    end
    if msg.media then
      	if msg.media.type == 'photo' and redis:get("bot:photo") then
      		if redis:get("bot:photo") == 'waiting' then
        		load_photo(msg.id, set_bot_photo, msg)
      		end
      	end
    end
    if matches[1] == "setbotphoto" then
    	redis:set("bot:photo", "waiting")
    	return 'Please send me bot photo now'
    end
    if matches[1] == "markread" then
    	if matches[2] == "on" then
    		redis:set("bot:markread", "on")
    		return "Mark read > on"
    	end
    	if matches[2] == "off" then
    		redis:del("bot:markread")
    		return "Mark read > off"
    	end
    	return
    end
    if matches[1] == "pm" then
    	send_large_msg("user#id"..matches[2],matches[3])
    	return "Msg sent"
    end
    if matches[1] == "block" then
    	if is_admin2(matches[2]) then
    		return "You can't block admins"
    	end
    	block_user("user#id"..matches[2],ok_cb,false)
    	return "User blocked"
    end
    if matches[1] == "unblock" then
    	unblock_user("user#id"..matches[2],ok_cb,false)
    	return "User unblocked"
    end
    if matches[1] == "import" then
    	local hash = parsed_url(matches[2])
    	import_chat_link(hash,ok_cb,false)
    end
    return
end
return {
  patterns = {
    "^[!/](pm) (%d+) (.*)$",
    "^[!/](import) (.*)$",
    "^[!/](unblock) (%d+)$",
    "^[!/](block) (%d+)$",
	"^[!/](markread) (on)$",
	"^[!/](markread) (off)$",
    "^[!/](setbotphoto)$",
	"%[(photo)%]"
  },
  run = run,
}