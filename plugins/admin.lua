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
local function get_contact_list_callback (cb_extra, success, result)
  local text = " "
  for k,v in pairs(result) do
    if v.print_name and v.id and v.phone then
      text = text..string.gsub(v.print_name ,  "_" , " ").." ["..v.id.."] = "..v.phone.."\n"
    end
  end
  local file = io.open("contact_list.txt", "w")
  file:write(text)
  file:flush()
  file:close()
  send_document("user#id"..cb_extra.target,"contact_list.txt", ok_cb, false)--.txt format
  local file = io.open("contact_list.json", "w")
  file:write(json:encode_pretty(result))
  file:flush()
  file:close()
  send_document("user#id"..cb_extra.target,"contact_list.json", ok_cb, false)--json format
end
local function user_info_callback(cb_extra, success, result)
  result.access_hash = nil
  result.flags = nil
  result.phone = nil
  if result.username then
    result.username = '@'..result.username
  end
  result.print_name = result.print_name:gsub("_","")
  local text = serpent.block(result, {comment=false})
  text = text:gsub("[{}]", "")
  text = text:gsub('"', "")
  text = text:gsub(",","")
  if cb_extra.msg.to.type == "chat" then
    send_large_msg("chat#id"..cb_extra.msg.to.id, text)
  else
    send_large_msg("user#id"..cb_extra.msg.to.id, text)
  end
end
local function get_dialog_list_callback(cb_extra, success, result)
  local text = ""
  for k,v in pairs(result) do
    if v.peer then
      if v.peer.type == "chat" then
        text = text.."group{"..v.peer.title.."}["..v.peer.id.."]("..v.peer.members_num..")"
      else
        if v.peer.print_name and v.peer.id then
          text = text.."user{"..v.peer.print_name.."}["..v.peer.id.."]"
        end
        if v.peer.username then
          text = text.."("..v.peer.username..")"
        end
        if v.peer.phone then
          text = text.."'"..v.peer.phone.."'"
        end
      end
    end
    if v.message then
      text = text..'\nlast msg >\nmsg id = '..v.message.id
      if v.message.text then
        text = text .. "\n text = "..v.message.text
      end
      if v.message.action then
        text = text.."\n"..serpent.block(v.message.action, {comment=false})
      end
      if v.message.from then
        if v.message.from.print_name then
          text = text.."\n From > \n"..string.gsub(v.message.from.print_name, "_"," ").."["..v.message.from.id.."]"
        end
        if v.message.from.username then
          text = text.."( "..v.message.from.username.." )"
        end
        if v.message.from.phone then
          text = text.."' "..v.message.from.phone.." '"
        end
      end
    end
    text = text.."\n\n"
  end
  local file = io.open("dialog_list.txt", "w")
  file:write(text)
  file:flush()
  file:close()
  send_document("user#id"..cb_extra.target,"dialog_list.txt", ok_cb, false)--.txt format
  local file = io.open("dialog_list.json", "w")
  file:write(json:encode_pretty(result))
  file:flush()
  file:close()
  send_document("user#id"..cb_extra.target,"dialog_list.json", ok_cb, false)--json format
end
local function res_callback(extra, success, result)
  local user = result.id
  local action = extra.action
  local receiver = extra.receiver
  if action == 'pm' then
    local text = extra.text
    send_large_msg("user#id"..user, text)
    send_large_msg(receiver, 'Message sent')
  elseif action == 'block' then
    block_user("user#id"..user, ok_cb, false)
    send_large_msg(receiver, 'User blocked')
  elseif action == 'unblock' then
    unblock_user("user#id"..user, ok_cb, false)
    send_large_msg(receiver, 'User unblocked')
  elseif action == 'del' then
    del_contact("user#id"..user, ok_cb, false)
    send_large_msg(receiver, 'User removed from contact list')
  end
  return user
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
    	if string.match(matches[2], '^%d+$') then
    	  send_large_msg("user#id"..matches[2], matches[3])
    	  return "Message sent"
    	else
    	  local username = matches[2]:gsub("@","")
    	  local text, action
    	  local res_extra = {text = matches[3], action = 'pm', receiver = receiver}
    	  return res_user(username,  res_callback, res_extra)
    	end
    end
    if matches[1] == "block" then
    	if is_admin2(matches[2]) then
    		return "You can't block admins"
    	end
    	if string.match(matches[2], '^%d+$') then
    	  block_user("user#id"..matches[2], ok_cb, false)
    	  return "User blocked"
    	else
    	  local username = matches[2]:gsub("@","")
    	  local text, action
    	  local res_extra = {text = '', action = 'block', receiver = receiver}
    	  return res_user(username, res_callback, res_extra)
    	end
    end
    if matches[1] == "unblock" then
    	if string.match(matches[2], '^%d+$') then
    	  unblock_user("user#id"..matches[2], ok_cb, false)
    	  return "User unblocked"
    	else
    	  local username = matches[2]:gsub("@","")
    	  local text, action
    	  local res_extra = {text = '', action = 'unblock', receiver = receiver}
    	  return res_user(username, res_callback, res_extra)
    	end
    end
    if matches[1] == "import" then--join by group link
    	local hash = parsed_url(matches[2])
    	import_chat_link(hash,ok_cb,false)
    end
    if matches[1] == "contactlist" then
      get_contact_list(get_contact_list_callback, {target = msg.from.id})
      return "I've sent contact list with both json and text format to your private"
    end
    if matches[1] == "delcontact" then
      if string.match(matches[2], '^%d+$') then
        del_contact("user#id"..matches[2],ok_cb,false)
        return "User "..matches[2].." removed from contact list"
      else
        local username = matches[2]:gsub("@","")
    	  local text, action
    	  local res_extra = {text = '', action = 'del', receiver = receiver}
    	  return res_user(username,  res_callback, res_extra)
    	end
    end
    if matches[1] == "dialoglist" then
      get_dialog_list(get_dialog_list_callback, {target = msg.from.id})
      return "I've sent dialog list with both json and text format to your private"
    end
    if matches[1] == "whois" then
      user_info("user#id"..matches[2],user_info_callback,{msg=msg})
    end
    return
end
return {
  patterns = {
	"^[!/](pm) (@?[%d%a]+) (.*)$",
	"^[!/](import) (.*)$",
	"^[!/](unblock) (@?[%d%a]+)$",
	"^[!/](block) (@?[%d%a]+)$",
	"^[!/](markread) (on)$",
	"^[!/](markread) (off)$",
	"^[!/](setbotphoto)$",
	"%[(photo)%]",
	"^[!/](contactlist)$",
	"^[!/](dialoglist)$",
	"^[!/](delcontact) (@?[%d%a]+)$",
	"^[!/](whois) (%d+)$"
  },
  run = run,
}
--By @imandaneshi :)
--https://github.com/SEEDTEAM/TeleSeed/blob/master/plugins/admin.lua
