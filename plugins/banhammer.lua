
local function pre_process(msg)
  -- SERVICE MESSAGE
  if msg.action and msg.action.type then
    local action = msg.action.type
    -- Check if banned user joins chat by link
    if action == 'chat_add_user_link' then
      local user_id = msg.from.id
      print('Checking invited user '..user_id)
      local banned = is_banned(user_id, msg.to.id)
      if banned or is_gbanned(user_id) then -- Check it with redis
      print('User is banned!')
      local name = user_print_name(msg.from)
      savelog(msg.to.id, name.." ["..msg.from.id.."] is banned and kicked ! ")-- Save to logs
      kick_user(user_id, msg.to.id)
      end
    end
    -- Check if banned user joins chat
    if action == 'chat_add_user' then
      local user_id = msg.action.user.id
      print('Checking invited user '..user_id)
      local banned = is_banned(user_id, msg.to.id)
      if banned or is_gbanned(user_id) then -- Check it with redis
        print('User is banned!')
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] added a banned user >"..msg.action.user.id)-- Save to logs
        kick_user(user_id, msg.to.id)
        local banhash = 'addedbanuser:'..msg.to.id..':'..msg.from.id
        redis:incr(banhash)
        local banhash = 'addedbanuser:'..msg.to.id..':'..msg.from.id
        local banaddredis = redis:get(banhash) 
        if banaddredis then 
          if tonumber(banaddredis) == 4 and not is_owner(msg) then 
            kick_user(msg.from.id, msg.to.id)-- Kick user who adds ban ppl more than 3 times
          end
          if tonumber(banaddredis) ==  8 and not is_owner(msg) then 
            ban_user(msg.from.id, msg.to.id)-- Kick user who adds ban ppl more than 7 times
            local banhash = 'addedbanuser:'..msg.to.id..':'..msg.from.id
            redis:set(banhash, 0)-- Reset the Counter
          end
        end
      end
      local bots_protection = "Yes"
      local data = load_data(_config.moderation.data)
      if data[tostring(msg.to.id)]['settings']['lock_bots'] then
        bots_protection = data[tostring(msg.to.id)]['settings']['lock_bots']
      end
    if msg.action.user.username ~= nil then
      if string.sub(msg.action.user.username:lower(), -3) == 'bot' and not is_momod(msg) and bots_protection == "yes" then --- Will kick bots added by normal users
        local name = user_print_name(msg.from)
          savelog(msg.to.id, name.." ["..msg.from.id.."] added a bot > @".. msg.action.user.username)-- Save to logs
          kick_user(msg.action.user.id, msg.to.id)
      end
    end
  end
    -- No further checks
  return msg
  end
  -- banned user is talking !
  if msg.to.type == 'chat' then
    local data = load_data(_config.moderation.data)
    local group = msg.to.id
    local texttext = 'groups'
    --if not data[tostring(texttext)][tostring(msg.to.id)] and not is_realm(msg) then -- Check if this group is one of my groups or not
    --chat_del_user('chat#id'..msg.to.id,'user#id'..our_id,ok_cb,false)
    --return 
    --end
    local user_id = msg.from.id
    local chat_id = msg.to.id
    local banned = is_banned(user_id, chat_id)
    if banned or is_gbanned(user_id) then -- Check it with redis
      print('Banned user talking!')
      local name = user_print_name(msg.from)
      savelog(msg.to.id, name.." ["..msg.from.id.."] banned user is talking !")-- Save to logs
      kick_user(user_id, chat_id)
      msg.text = ''
    end
  end
  return msg
end

local function username_id(cb_extra, success, result)
  local get_cmd = cb_extra.get_cmd
  local receiver = cb_extra.receiver
  local chat_id = cb_extra.chat_id
  local member = cb_extra.member
  local text = ''
  for k,v in pairs(result.members) do
    vusername = v.username
    if vusername == member then
      member_username = member
      member_id = v.id
      if member_id == our_id then return false end
      if get_cmd == 'kick' then
        if is_momod2(member_id, chat_id) then
          return send_large_msg(receiver, "you can't kick mods/owner/admins")
        end
        return kick_user(member_id, chat_id)
      elseif get_cmd == 'ban' then
        if is_momod2(member_id, chat_id) then
          return send_large_msg(receiver, "you can't ban mods/owner/admins")
        end
        send_large_msg(receiver, 'User @'..member..' ['..member_id..'] banned')
        return ban_user(member_id, chat_id)
      elseif get_cmd == 'unban' then
        send_large_msg(receiver, 'User @'..member..' ['..member_id..'] unbanned')
        local hash =  'banned:'..chat_id
        redis:srem(hash, member_id)
        return 'User '..user_id..' unbanned'
      elseif get_cmd == 'banall' then
        send_large_msg(receiver, 'User @'..member..' ['..member_id..'] globally banned')
        return banall_user(member_id, chat_id)
      elseif get_cmd == 'unbanall' then
        send_large_msg(receiver, 'User @'..member..' ['..member_id..'] unbanned')
        return unbanall_user(member_id, chat_id)
      end
    end
  end
  return send_large_msg(receiver, text)
end
local function run(msg, matches)
 if matches[1]:lower() == 'id' then
    if msg.to.type == "user" then
      return "Bot ID: "..msg.to.id.. "\n\nYour ID: "..msg.from.id
    end
    if type(msg.reply_id) ~= "nil" then
      local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] used /id ")
      id = get_message(msg.reply_id,get_message_callback_id, false)
    elseif matches[1]:lower() == 'id' then
      local name = user_print_name(msg.from)
      savelog(msg.to.id, name.." ["..msg.from.id.."] used /id ")
      return "Group ID for " ..string.gsub(msg.to.print_name, "_", " ").. ":\n\n"..msg.to.id  
    end
  end
  local receiver = get_receiver(msg)
  if matches[1]:lower() == 'kickme' then-- /kickme
    if msg.to.type == 'chat' then
      local name = user_print_name(msg.from)
      savelog(msg.to.id, name.." ["..msg.from.id.."] left using kickme ")-- Save to logs
      chat_del_user("chat#id"..msg.to.id, "user#id"..msg.from.id, ok_cb, false)
    end
  end
  if not is_momod(msg) then -- Ignore normal users 
    return nil
  end

  if matches[1]:lower() == "banlist" then -- Ban list !
    local chat_id = msg.to.id
    if matches[2] and is_admin(msg) then
      chat_id = matches[2] 
    end
    return ban_list(chat_id)
  end
  if matches[1]:lower() == 'ban' then-- /ban 
    if type(msg.reply_id)~="nil" and is_momod(msg) then
      if is_admin(msg) then
        local msgr = get_message(msg.reply_id,ban_by_reply_admins, false)
      else
        msgr = get_message(msg.reply_id,ban_by_reply, false)
      end
    end
    if msg.to.type == 'chat' then
      local user_id = matches[2]
      local chat_id = msg.to.id
      if string.match(matches[2], '^%d+$') then
        if tonumber(matches[2]) == tonumber(our_id) then 
          return
        end
        if not is_admin(msg) and is_momod2(tonumber(matches[2]), msg.to.id) then
          return "you can't ban mods/owner/admins"
        end
        if tonumber(matches[2]) == tonumber(msg.from.id) then
          return "You can't ban your self !"
        end
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] baned user ".. matches[2])
        ban_user(user_id, chat_id)
      else
        local member = string.gsub(matches[2], '@', '')
        local get_cmd = 'ban'
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] baned user ".. matches[2])
        chat_info(receiver, username_id, {get_cmd=get_cmd, receiver=receiver, chat_id=msg.to.id, member=member})
      end
    return 
    end
  end
  if matches[1]:lower() == 'unban' then -- /unban 
    if type(msg.reply_id)~="nil" and is_momod(msg) then
      local msgr = get_message(msg.reply_id,unban_by_reply, false)
    end
    if msg.to.type == 'chat' then
      local user_id = matches[2]
      local chat_id = msg.to.id
      local targetuser = matches[2]
      if string.match(targetuser, '^%d+$') then
        local user_id = targetuser
        local hash =  'banned:'..chat_id
        redis:srem(hash, user_id)
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] unbaned user ".. matches[2])
        return 'User '..user_id..' unbanned'
      else
        local member = string.gsub(matches[2], '@', '')
        local get_cmd = 'unban'
        chat_info(receiver, username_id, {get_cmd=get_cmd, receiver=receiver, chat_id=msg.to.id, member=member})
      end
    end
  end

  if matches[1]:lower() == 'kick' then
    if type(msg.reply_id)~="nil" and is_momod(msg) then
      if is_admin(msg) then
        local msgr = get_message(msg.reply_id,Kick_by_reply_admins, false)
      else
        msgr = get_message(msg.reply_id,Kick_by_reply, false)
      end
    end
    if msg.to.type == 'chat' then
      local user_id = matches[2]
      local chat_id = msg.to.id
      if string.match(matches[2], '^%d+$') then
        if tonumber(matches[2]) == tonumber(our_id) then 
          return
        end
        if not is_admin(msg) and is_momod2(matches[2], msg.to.id) then
          return "you can't kick mods/owner/admins"
        end
        if tonumber(matches[2]) == tonumber(msg.from.id) then
          return "You can't kick your self !"
        end
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] kicked user ".. matches[2])
        kick_user(user_id, chat_id)
      else
        local member = string.gsub(matches[2], '@', '')
        local get_cmd = 'kick'
        local name = user_print_name(msg.from)
        savelog(msg.to.id, name.." ["..msg.from.id.."] kicked user ".. matches[2])
        chat_info(receiver, username_id, {get_cmd=get_cmd, receiver=receiver, chat_id=msg.to.id, member=member})
      end
    else
      return 'This isn\'t a chat group'
    end
  end

  if not is_admin(msg) then
    return
  end

  if matches[1]:lower() == 'banall' then -- Global ban
    if type(msg.reply_id) ~="nil" and is_admin(msg) then
      return get_message(msg.reply_id,banall_by_reply, false)
    end
    local user_id = matches[2]
    local chat_id = msg.to.id
    if msg.to.type == 'chat' then
      local targetuser = matches[2]
      if string.match(targetuser, '^%d+$') then
        if tonumber(matches[2]) == tonumber(our_id) then
         return false 
        end
        banall_user(targetuser)
        return 'User ['..user_id..' ] globally banned'
      else
        local member = string.gsub(matches[2], '@', '')
        local get_cmd = 'banall'
        chat_info(receiver, username_id, {get_cmd=get_cmd, receiver=receiver, chat_id=msg.to.id, member=member})
      end
    end
  end
  if matches[1]:lower() == 'unbanall' then -- Global unban
    local user_id = matches[2]
    local chat_id = msg.to.id
    if msg.to.type == 'chat' then
      if string.match(matches[2], '^%d+$') then
        if tonumber(matches[2]) == tonumber(our_id) then 
          return false 
        end
        unbanall_user(user_id)
        return 'User ['..user_id..' ] removed from global ban list'
      else
        local member = string.gsub(matches[2], '@', '')
        local get_cmd = 'unbanall'
        chat_info(receiver, username_id, {get_cmd=get_cmd, receiver=receiver, chat_id=msg.to.id, member=member})
      end
    end
  end
  if matches[1]:lower() == "gbanlist" then -- Global ban list
    return banall_list()
  end
end

return {
  patterns = {
    "^[!/]([Bb]anall) (.*)$",
    "^[!/]([Bb]anall)$",
    "^[!/]([Bb]anlist) (.*)$",
    "^[!/]([Bb]anlist)$",
    "^[!/]([Gg]banlist)$",
    "^[!/]([Bb]an) (.*)$",
    "^[!/]([Kk]ick)$",
    "^[!/]([Uu]nban) (.*)$",
    "^[!/]([Uu]nbanall) (.*)$",
    "^[!/]([Uu]nbanall)$",
    "^[!/]([Kk]ick) (.*)$",
    "^[!/]([Kk]ickme)$",
    "^[!/]([Bb]an)$",
    "^[!/]([Uu]nban)$",
    "^[!/]([Ii]d)$",
    "^!!tgservice (.+)$",
  },
  run = run,
  pre_process = pre_process
}
