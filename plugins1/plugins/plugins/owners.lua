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

local function lock_group_arabic(msg, data, target)
  local group_arabic_lock = data[tostring(target)]['settings']['lock_arabic']
  if group_arabic_lock == 'yes' then
    return 'Arabic is already locked'
  else
    data[tostring(target)]['settings']['lock_arabic'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Arabic has been locked'
  end
end

local function unlock_group_arabic(msg, data, target)
  local group_arabic_lock = data[tostring(target)]['settings']['lock_arabic']
  if group_arabic_lock == 'no' then
    return 'Arabic/Persian is already unlocked'
  else
    data[tostring(target)]['settings']['lock_arabic'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Arabic/Persian has been unlocked'
  end
end

local function lock_group_links(msg, data, target)
  local group_link_lock = data[tostring(target)]['settings']['lock_link']
  if group_link_lock == 'yes' then
    return 'Link posting is already locked'
  else
    data[tostring(target)]['settings']['lock_link'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Link posting has been locked'
  end
end

local function unlock_group_links(msg, data, target)
  local group_link_lock = data[tostring(target)]['settings']['lock_link']
  if group_link_lock == 'no' then
    return 'Link posting is not locked'
  else
    data[tostring(target)]['settings']['lock_link'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Link posting has been unlocked'
  end
end

local function lock_group_spam(msg, data, target)

  local group_spam_lock = data[tostring(target)]['settings']['lock_spam']
  if group_spam_lock == 'yes' then
    return 'SuperGroup spam is already locked'
  else
    data[tostring(target)]['settings']['lock_spam'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'SuperGroup spam has been locked'
  end
end

local function unlock_group_spam(msg, data, target)

  local group_spam_lock = data[tostring(target)]['settings']['lock_spam']
  if group_spam_lock == 'no' then
    return 'SuperGroup spam is not locked'
  else
    data[tostring(target)]['settings']['lock_spam'] = 'no'
    save_data(_config.moderation.data, data)
    return 'SuperGroup spam has been unlocked'
  end
end

local function lock_group_sticker(msg, data, target)

  local group_sticker_lock = data[tostring(target)]['settings']['lock_sticker']
  if group_sticker_lock == 'yes' then
    return 'Sticker posting is already locked'
  else
    data[tostring(target)]['settings']['lock_sticker'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Sticker posting has been locked'
  end
end

local function unlock_group_sticker(msg, data, target)

  local group_sticker_lock = data[tostring(target)]['settings']['lock_sticker']
  if group_sticker_lock == 'no' then
    return 'Sticker posting is already unlocked'
  else
    data[tostring(target)]['settings']['lock_sticker'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Sticker posting has been unlocked'
  end
end

local function lock_group_contacts(msg, data, target)
  if not is_momod(msg) then
    return
  end
  local group_rtl_lock = data[tostring(target)]['settings']['lock_contacts']
  if group_contacts_lock == 'yes' then
    return 'Contact posting is already locked'
  else
    data[tostring(target)]['settings']['lock_contacts'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Contact posting has been locked'
  end
end

local function unlock_group_contacts(msg, data, target)
  if not is_momod(msg) then
    return
  end
  local group_contacts_lock = data[tostring(target)]['settings']['lock_contacts']
  if group_contacts_lock == 'no' then
    return 'Contact posting is already unlocked'
  else
    data[tostring(target)]['settings']['lock_contacts'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Contact posting has been unlocked'
  end
end

local function enable_strict_rules(msg, data, target)
  if not is_momod(msg) then
    return
  end
  local group_rtl_lock = data[tostring(target)]['settings']['strict']
  if strict == 'yes' then
    return 'Settings are already strictly enforced'
  else
    data[tostring(target)]['settings']['strict'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Settings will be strictly enforced'
  end
end

local function disable_strict_rules(msg, data, target)
  if not is_momod(msg) then
    return
  end
  local group_contacts_lock = data[tostring(target)]['settings']['strict']
  if strict == 'no' then
    return 'Settings will not be strictly enforced'
  else
    data[tostring(target)]['settings']['strict'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Settings are not strictly enforced'
  end
end

-- Show group settings
local function show_group_settingsmod(msg, data, target)
    if data[tostring(target)] then
      if data[tostring(target)]['settings']['flood_msg_max'] then
        NUM_MSG_MAX = tonumber(data[tostring(target)]['settings']['flood_msg_max'])
        print('custom'..NUM_MSG_MAX)
      else 
        NUM_MSG_MAX = 5
      end
    end
	local bots_protection = "Yes"
    if data[tostring(target)]['settings']['lock_bots'] then
    	bots_protection = data[tostring(target)]['settings']['lock_bots']
   	end
    local leave_ban = "no"
    if data[tostring(target)]['settings']['leave_ban'] then
    	leave_ban = data[tostring(target)]['settings']['leave_ban']
   	end
	local public = "no"
	if data[tostring(target)]['settings'] then
		if data[tostring(target)]['settings']['public'] then
			public = data[tostring(target)]['settings']['public']
		end
	end
    local settings = data[tostring(target)]['settings']
    local text = "Group settings:\nLock group name : "..settings.lock_name.."\nLock group photo : "..settings.lock_photo.."\nLock group member : "..settings.lock_member.."\nLock group leave : "..leave_ban.."\nflood sensitivity : "..NUM_MSG_MAX.."\nBot protection : "..bots_protection.."\nPublic: "..public
    return text
end

-- Show SuperGroup settings
local function show_super_group_settings(msg, data, target)
    if data[tostring(target)] then
      if data[tostring(target)]['settings']['flood_msg_max'] then
        NUM_MSG_MAX = tonumber(data[tostring(target)]['settings']['flood_msg_max'])
        print('custom'..NUM_MSG_MAX)
      else 
        NUM_MSG_MAX = 5
      end
    end
	if data[tostring(target)]['settings'] then
		if not data[tostring(target)]['settings']['public'] then
			data[tostring(target)]['settings']['public'] = 'no'
		end
	end
	if data[tostring(target)]['settings'] then
		if not data[tostring(target)]['settings']['lock_rtl'] then
			data[tostring(target)]['settings']['lock_rtl'] = 'no'
		end
	end
	if data[tostring(target)]['settings'] then
		if not data[tostring(target)]['settings']['lock_member'] then
			data[tostring(target)]['settings']['lock_member'] = 'no'
		end
	end
    local settings = data[tostring(target)]['settings']
    local text = "SuperGroup settings for "..target..":\nLock links : "..settings.lock_link.."\nLock flood: "..settings.flood.."\nLock spam: "..settings.lock_spam.."\nLock Arabic: "..settings.lock_arabic.."\nLock Member: "..settings.lock_member.."\nLock RTL: "..settings.lock_rtl.."\nLock sticker: "..settings.lock_sticker.."\nPublic: "..settings.public.."\nStrict settings: "..settings.strict
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
  if msg.to.type == 'user' then
    local print_name = user_print_name(msg.from):gsub("‮", "")
	local name = print_name:gsub("_", " ")
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
      kick_user(matches[3], chat_id)
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
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned modlist")
      end
      if matches[3] == 'rules' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local data_cat = 'rules'
        data[tostring(matches[1])][data_cat] = nil
        save_data(_config.moderation.data, data)
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned rules")
      end
      if matches[3] == 'about' then
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local data_cat = 'description'
        data[tostring(matches[1])][data_cat] = nil
        save_data(_config.moderation.data, data)
        savelog(matches[1], name.." ["..msg.from.id.."] cleaned about")
		channel_set_about(receiver, about_text, ok_cb, false)
		return "About has been cleaned"
      end
	  	if matches[3] == 'mutelist' then
			chat_id = string.match(matches[1], '^%d+$')
			local hash =  'mute_user:'..chat_id
			redis:del(hash)
			return "Mutelist Cleaned"
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
      savelog(matches[1], name.." ["..msg.from.id.."] set flood to ["..matches[3].."]")
      return 'Group flood has been set to '..matches[3]
    end
	
    if matches[2] == 'lock' then
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      local target = matches[1]
	  local group_type = data[tostring(matches[1])]['group_type']
      if matches[3] == 'name' then
        savelog(matches[1], name.." ["..msg.from.id.."] locked name ")
        return lock_group_namemod(msg, data, target)
      end
      if matches[3] == 'member' then
        savelog(matches[1], name.." ["..msg.from.id.."] locked member ")
        return lock_group_membermod(msg, data, target)
      end
	  if matches[3] == 'arabic' then
		savelog(matches[1], name.." ["..msg.from.id.."] locked arabic ")
		return lock_group_arabic(msg, data, target)
	  end
	  if matches[3] == 'links' then
		savelog(matches[1], name.." ["..msg.from.id.."] locked links ")
		return lock_group_links(msg, data, target)
	  end
	  if matches[3] == 'spam' then
	    savelog(matches[1], name.." ["..msg.from.id.."] locked spam ")
		return lock_group_spam(msg, data, target)
	  end
	  if matches[3] == 'rtl' then
	    savelog(matches[1], name.." ["..msg.from.id.."] locked RTL chars. in names")
		return unlock_group_rtl(msg, data, target)
	  end
	  if matches[3] == 'sticker' then
		savelog(matches[1], name.." ["..msg.from.id.."] locked sticker")
		return lock_group_sticker(msg, data, target)
	  end
	  
    end
	
    if matches[2] == 'unlock' then
      if not is_owner2(msg.from.id, chat_id) then
        return "You are not the owner of this group"
      end
      local target = matches[1]
	  local group_type = data[tostring(matches[1])]['group_type']
      if matches[3] == 'name' then
        savelog(matches[1], name.." ["..msg.from.id.."] unlocked name ")
        return unlock_group_namemod(msg, data, target)
      end
      if matches[3] == 'member' then
        savelog(matches[1], name.." ["..msg.from.id.."] unlocked member ")
        return unlock_group_membermod(msg, data, target)
      end
	  if matches[3] == 'arabic' then
		savelog(matches[1], name.." ["..msg.from.id.."] unlocked arabic ")
		return unlock_group_arabic(msg, data, target)
	  end
	  if matches[3] == 'links' and group_type == "SuperGroup" then
		savelog(matches[1], name.." ["..msg.from.id.."] unlocked links ")
		return unlock_group_links(msg, data, target)
	  end
	  if matches[3] == 'spam' and group_type == "SuperGroup" then
	    savelog(matches[1], name.." ["..msg.from.id.."] unlocked spam ")
		return unlock_group_spam(msg, data, target)
	  end
	  if matches[3] == 'rtl' then
	    savelog(matches[1], name.." ["..msg.from.id.."] unlocked RTL chars. in names")
		return unlock_group_rtl(msg, data, target)
	  end
	  if matches[3] == 'sticker' and group_type == "SuperGroup" then
		savelog(matches[1], name.." ["..msg.from.id.."] unlocked sticker")
		return unlock_group_sticker(msg, data, target)
	  end
	  if matches[3] == 'contacts' and group_type == "SuperGroup" then
		savelog(matches[1], name_log.." ["..msg.from.id.."] locked contact posting")
		return lock_group_contacts(msg, data, target)
	  end
	  if matches[3] == 'strict' and group_type == "SuperGroup" then
		savelog(matches[1], name_log.." ["..msg.from.id.."] locked enabled strict settings")
		return enable_strict_rules(msg, data, target)
	  end
    end
	
	if matches[2] == 'new' then
      if matches[3] == 'link' then
	   local group_type = data[tostring(matches[1])]['group_type']
        if not is_owner2(msg.from.id, chat_id) then
          return "You are not the owner of this group"
        end
        local function callback_grouplink (extra , success, result)
			local receiver = 'chat#id'..matches[1]
		    if success == 0 then
				send_large_msg(receiver, '*Error: Failed to retrieve link* \nReason: Not creator.')
		    end
				data[tostring(matches[1])]['settings']['set_link'] = result
				save_data(_config.moderation.data, data)
				return
		end
		local function callback_superlink (extra , success, result)
			vardump(result)
			local receiver = 'channel#id'..matches[1]
			local user = extra.user
			if success == 0 then
				data[tostring(matches[1])]['settings']['set_link'] = nil
				save_data(_config.moderation.data, data)
				return send_large_msg(user, '*Error: Failed to retrieve link* \nReason: Not creator.\n\nIf you have the link, please use /setlink to set it')
			else
				data[tostring(matches[1])]['settings']['set_link'] = result
				save_data(_config.moderation.data, data)
				return send_large_msg(user, "Created a new link")
			end	
		end
		if group_type == "Group" then
			local receiver = 'chat#id'..matches[1]
			savelog(matches[1], name.." ["..msg.from.id.."] created/revoked group link ")
			export_chat_link(receiver, callback_grouplink, false)
			return "Created a new new link ! \n owner can get it by /owners "..matches[1].." get link"
		elseif group_type == "SuperGroup" then
			local receiver = 'channel#id'..matches[1]
			local user = 'user#id'..msg.from.id
			savelog(matches[1], name.." ["..msg.from.id.."] attempted to create a new SuperGroup link")
			export_channel_link(receiver, callback_superlink, {user = user})
		end
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
        savelog(matches[1], name.." ["..msg.from.id.."] requested group link ["..group_link.."]")
        return "Group link:\n"..group_link
      end
    end
	
    if matches[1] == 'changeabout' and matches[2] then
		if not is_owner2(msg.from.id, matches[2]) then
          return "You are not the owner of this group"
        end
		local group_type = data[tostring(matches[2])]['group_type']
		if group_type == "Group" or group_type == "Realm" then
			local target = matches[2]
			local about = matches[3]
			local name = user_print_name(msg.from)
			savelog(matches[2], name.." ["..msg.from.id.."] has changed group description to ["..matches[3].."]")
			return set_description(target, about)
		elseif group_type == "SuperGroup" then
			local channel = 'channel#id'..matches[2]
			local about_text = matches[3]
			local data_cat = 'description'
			local target = matches[2]
			channel_set_about(channel, about_text, ok_cb, false)
			data[tostring(target)][data_cat] = about_text
			save_data(_config.moderation.data, data)
			savelog(matches[2], name.." ["..msg.from.id.."] has changed SuperGroup description to ["..matches[3].."]")
			return "Description has been set for ["..matches[2]..']'
		end
    end
	
	if matches[1] == 'viewsettings' and data[tostring(matches[2])]['settings'] then
	    if not is_owner2(msg.from.id, matches[2]) then
          return "You are not the owner of this group"
        end
		local target = matches[2]
		local group_type = data[tostring(matches[2])]['group_type']
		if group_type == "Group" or group_type == "Realm" then
			savelog(matches[2], name.." ["..msg.from.id.."] requested group settings ")
			return show_group_settings(msg, data, target)
		elseif group_type == "SuperGroup" then
			savelog(matches[2], name.." ["..msg.from.id.."] requested SuperGroup settings ")
			return show_super_group_settings(msg, data, target)
		end
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
	  local group_name_set = data[tostring(matches[2])]['settings']['set_name']
      save_data(_config.moderation.data, data)
	  local chat_to_rename = 'chat#id'..matches[2]
	  local channel_to_rename = 'channel#id'..matches[2]
      savelog(matches[2], "Group name changed to [ "..new_name.." ] by "..name.." ["..msg.from.id.."]")
      rename_chat(chat_to_rename, group_name_set, ok_cb, false)
	  rename_channel(channel_to_rename, group_name_set, ok_cb, false)
    end
	
    if matches[1] == 'loggroup' and matches[2] and is_owner2(msg.from.id, matches[2]) then
      savelog(matches[2], "log file created by owner/support/admin")
      send_document("user#id".. msg.from.id,"./groups/logs/"..matches[2].."log.txt", ok_cb, false)
    end
  end
end

return {
  patterns = {
    "^[#!/]owners (%d+) ([^%s]+) (.*)$",
    "^[#!/]owners (%d+) ([^%s]+)$",
    "^[#!/](changeabout) (%d+) (.*)$",
    "^[#!/](changerules) (%d+) (.*)$",
    "^[#!/](changename) (%d+) (.*)$",
    "^[#!/](viewsettings) (%d+)$",
	"^[#!/](loggroup) (%d+)$"
  },
  run = run
}