

local function lock_group_namemod(msg, data, target)
  local group_name_set = data[tostring(target)]['settings']['set_name']
  local group_name_lock = data[tostring(target)]['settings']['lock_name']
  if group_name_lock == 'yes' then
    return 'Group name is already locked'
  else
    data[tostring(target)]['settings']['lock_name'] = 'yes'
    save_data(_config.moderation.data, data)
    rename_chat('chat#id'..target, group_name_set, ok_cb, false)
  return 'Group name has been locked'
  end
end

local function unlock_group_namemod(msg, data, target)
  local group_name_set = data[tostring(target)]['settings']['set_name']
  local group_name_lock = data[tostring(target)]['settings']['lock_name']
  if group_name_lock == 'no' then
    return 'Group name is already unlocked'
  else
    data[tostring(target)]['settings']['lock_name'] = 'no'
    save_data(_config.moderation.data, data)
  return 'Group name has been unlocked'
  end
end

local function lock_group_floodmod(msg, data, target)
  local group_flood_lock = data[tostring(target)]['settings']['flood']
  if group_flood_lock == 'yes' then
    return 'Group flood is locked'
  else
    data[tostring(target)]['settings']['flood'] = 'yes'
    save_data(_config.moderation.data, data)
  return 'Group flood has been locked'
  end
end

local function unlock_group_floodmod(msg, data, target)
  local group_flood_lock = data[tostring(target)]['settings']['flood']
  if group_flood_lock == 'no' then
    return 'Group flood is not locked'
  else
    data[tostring(target)]['settings']['flood'] = 'no'
    save_data(_config.moderation.data, data)
  return 'Group flood has been unlocked'
  end
end

local function lock_group_membermod(msg, data, target)
  local group_member_lock = data[tostring(target)]['settings']['lock_member']
  if group_member_lock == 'yes' then
    return 'Group members are already locked'
  else
    data[tostring(target)]['settings']['lock_member'] = 'yes'
    save_data(_config.moderation.data, data)
  end
  return 'Group members has been locked'
end

local function unlock_group_membermod(msg, data, target)
  local group_member_lock = data[tostring(target)]['settings']['lock_member']
  if group_member_lock == 'no' then
    return 'Group members are not locked'
  else
    data[tostring(target)]['settings']['lock_member'] = 'no'
    save_data(_config.moderation.data, data)
  return 'Group members has been unlocked'
  end
end

local function unlock_group_photomod(msg, data, target)
  local group_photo_lock = data[tostring(target)]['settings']['lock_photo']
  if group_photo_lock == 'no' then
      return 'Group photo is not locked'
  else
      data[tostring(target)]['settings']['lock_photo'] = 'no'
      save_data(_config.moderation.data, data)
  return 'Group photo has been unlocked'
  end
end

local function show_group_settingsmod(msg, data, target)
    local data = load_data(_config.moderation.data)
    if data[tostring(msg.to.id)] then
      if data[tostring(msg.to.id)]['settings']['flood_msg_max'] then
        NUM_MSG_MAX = tonumber(data[tostring(msg.to.id)]['settings']['flood_msg_max'])
        print('custom'..NUM_MSG_MAX)
      else 
        NUM_MSG_MAX = 5
      end
    end
    local settings = data[tostring(target)]['settings']
    local text = "Group settings:\nLock group name : "..settings.lock_name.."\nLock group photo : "..settings.lock_photo.."\nLock group member : "..settings.lock_member.."\nflood sensitivity : "..NUM_MSG_MAX
    return text
end

local function set_rules(target, rules)
  local data = load_data(_config.moderation.data)
  local data_cat = 'rules'
  data[tostring(target)][data_cat] = rules
  save_data(_config.moderation.data, data)
  return 'Set group rules to:\n'..rules
end

local function set_description(target, about)
  local data = load_data(_config.moderation.data)
  local data_cat = 'description'
  data[tostring(target)][data_cat] = about
  save_data(_config.moderation.data, data)
  return 'Set group description to:\n'..about
end

local function run(msg, matches)
  if msg.to.type ~= 'chat' then
    local chat_id = matches[1]
    local receiver = get_receiver(msg)
    local data = load_data(_config.moderation.data)
    if matches[2] == 'ban' then
      local chat_id = matches[1]
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      if tonumber(matches[3]) == tonumber(our_id) then return false end
      local user_id = matches[3]
      if tonumber(matches[3]) == tonumber(msg.from.id) then 
        return "You can't ban yourself"
      end
      ban_user(matches[3], matches[1])
      local name = user_print_name(msg.from)
      savelog(matches[1], name.." ["..msg.from.id.."] banned user ".. matches[3])
      return 'User '..user_id..' banned'
    end
    if matches[2] == 'unban' then
    if tonumber(matches[3]) == tonumber(our_id) then return false end
      local chat_id = matches[1]
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      local user_id = matches[3]
      if tonumber(matches[3]) == tonumber(msg.from.id) then 
        return "You can't unban yourself"
      end
      local hash =  'banned:'..matches[1]
      redis:srem(hash, user_id)
      local name = user_print_name(msg.from)
      savelog(matches[1], name.." ["..msg.from.id.."] unbanned user ".. matches[3])
      return 'User '..user_id..' unbanned'
    end
    if matches[2] == 'kick' then
      local chat_id = matches[1]
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      if tonumber(matches[3]) == tonumber(our_id) then return false end
      local user_id = matches[3]
      if tonumber(matches[3]) == tonumber(msg.from.id) then 
        return "You can't kick yourself"
      end
      kick_user(matches[3], matches[1])
      local name = user_print_name(msg.from)
      savelog(matches[1], name.." ["..msg.from.id.."] kicked user ".. matches[3])
      return 'User '..user_id..' kicked'
    end
    if matches[2] == 'clean' then
      if matches[3] == 'modlist' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        for k,v in pairs(data[tostring(matches[1])]['moderators']) do
          data[tostring(matches[1])]['moderators'][tostring(k)] = nil
          save_data(_config.moderation.data, data)
        end
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned modlist")
      end
      if matches[3] == 'rules' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local data_cat = 'rules'
        data[tostring(matches[1])][data_cat] = nil
        save_data(_config.moderation.data, data)
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned rules")
      end
      if matches[3] == 'about' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local data_cat = 'description'
        data[tostring(matches[1])][data_cat] = nil
        save_data(_config.moderation.data, data)
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned about")
      end
    end
    if matches[2] == "setflood" then
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      if tonumber(matches[3]) < 5 or tonumber(matches[3]) > 20 then
        return "Wrong number,range is [5-20]"
      end
      local flood_max = matches[3]
      data[tostring(matches[1])]['settings']['flood_msg_max'] = flood_max
      save_data(_config.moderation.data, data)
      local name = user_print_name(msg.from)
      savelog(matches[1], name.." ["..msg.from.id.."] set flood to ["..matches[3].."]")
      return 'Group flood has been set to '..matches[3]
    end
    if matches[2] == 'lock' then
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      local target = matches[1]
      if matches[3] == 'name' then
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] locked name ")
        return lock_group_namemod(msg, data, target)
      end
      if matches[3] == 'member' then
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] locked member ")
        return lock_group_membermod(msg, data, target)
      end
    end
    if matches[2] == 'unlock' then
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      local target = matches[1]
      if matches[3] == 'name' then
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] unlocked name ")
        return unlock_group_namemod(msg, data, target)
      end
      if matches[3] == 'member' then
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] unlocked member ")
        return unlock_group_membermod(msg, data, target)
      end
    end
    if matches[2] == 'new' then
      if matches[3] == 'link' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local function callback (extra , success, result)
          local receiver = 'chat#'..matches[1]
          vardump(result)
          data[tostring(matches[1])]['settings']['set_link'] = result
          save_data(_config.moderation.data, data)
          return 
        end
        local receiver = 'chat#'..matches[1]
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] revoked group link ")
        export_chat_link(receiver, callback, true)
        return "Created a new new link ! \n owner can get it by /owners "..matches[1].." get link"
      end
    end
    if matches[2] == 'get' then 
      if matches[3] == 'link' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local group_link = data[tostring(matches[1])]['settings']['set_link']
        if not group_link then 
          return "Create a link using /newlink first !"
        end
        local name = user_print_name(msg.from)
        savelog(matches[1], name.." ["..msg.from.id.."] requested group link ["..group_link.."]")
        return "Group link:\n"..group_link
      end
    end
    if matches[1] == 'changeabout' and matches[2] and is_owner2(msg.from.id, matches[2]) then
      local target = matches[2]
      local about = matches[3]
      local name = user_print_name(msg.from)
      savelog(matches[2], name.." ["..msg.from.id.."] has changed group description to ["..matches[3].."]")
      return set_description(target, about)
    end
    if matches[1] == 'changerules' and is_owner2(msg.from.id, matches[2]) then
      local rules = matches[3]
      local target = matches[2]
      local name = user_print_name(msg.from)
      savelog(matches[2], name.." ["..msg.from.id.."] has changed group rules to ["..matches[3].."]")
      return set_rules(target, rules)
    end
    if matches[1] == 'changename' and is_owner2(msg.from.id, matches[2]) then
      local new_name = string.gsub(matches[3], '_', ' ')
      data[tostring(matches[2])]['settings']['set_name'] = new_name
      save_data(_config.moderation.data, data)
      local group_name_set = data[tostring(matches[2])]['settings']['set_name']
      local to_rename = 'chat#id'..matches[2]
      local name = user_print_name(msg.from)
      savelog(matches[2], "Group {}  name changed to [ "..new_name.." ] by "..name.." ["..msg.from.id.."]")
      rename_chat(to_rename, group_name_set, ok_cb, false)
    end
    if matches[1] == 'loggroup' and matches[2] and is_owner2(msg.from.id, matches[2]) then
      savelog(matches[2], "------")
      send_document("user#id".. msg.from.id,"./groups/logs/"..matches[2].."log.txt", ok_cb, false)
    end
  end
end
return {
  patterns = {
    "^[!/]owners (%d+) ([^%s]+) (.*)$",
    "^[!/]owners (%d+) ([^%s]+)$",
    "^[!/](changeabout) (%d+) (.*)$",
    "^[!/](changerules) (%d+) (.*)$",
    "^[!/](changename) (%d+) (.*)$",
		"^[!/](loggroup) (%d+)$"
  },
  run = run
}
