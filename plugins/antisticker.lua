-- data saved to data/moderation.json
do

local administrators_only = 'For administrator only!'
local moderators_only = 'For moderators only!'

local function create_group(msg)
  if not is_admin(msg) then return administrators_only end
  local group_creator = msg.from.print_name
  create_group_chat (group_creator, group_name, ok_cb, false)
	return 'Group '..string.gsub(group_name, '_', ' ')..' has been created.'
end

local function addgroup(msg)
  if not is_admin(msg) then return administrators_only end
  local data = load_data(_config.moderation.data)
  if data[tostring(msg.to.id)] then
    return 'Group is already added.'
  end
    -- create data array in moderation.json
  data[tostring(msg.to.id)] = {
    moderators ={},
    settings = {
      set_name = string.gsub(msg.to.print_name, '_', ' '),
      lock_bots = 'no',
      lock_name = 'no',
      lock_photo = 'no',
      lock_member = 'no',
      anti_flood = 'no',
      welcome = 'no',
      sticker = 'ok'
      }
    }
  save_data(_config.moderation.data, data)

  return 'Group has been added.'
end

local function remgroup(msg)
  if not is_admin(msg) then return administrators_only end
  local data = load_data(_config.moderation.data)
  local receiver = get_receiver(msg)
  if not data[tostring(msg.to.id)] then
    return 'Group is not added.'
  end

  data[tostring(msg.to.id)] = nil
  save_data(_config.moderation.data, data)

  return 'Group has been removed'
end

local function export_chat_link_callback(extra, success, result)
  local msg = extra.msg
  local group_name = msg.to.title
  local data = extra.data
  local receiver = get_receiver(msg)
  if success == 0 then
    return send_large_msg(receiver, 'Cannot generate invite link for this group.\nMake sure you are an admin or a sudoer.')
  end
  data[tostring(msg.to.id)]['link'] = result
  save_data(_config.moderation.data, data)
  return send_large_msg(receiver,'Newest generated invite link for '..group_name..' is:\n'..result)
end

local function set_description(msg, data)
  if not is_mod(msg) then return moderators_only end
  local data_cat = 'description'
	data[tostring(msg.to.id)][data_cat] = deskripsi
	save_data(_config.moderation.data, data)

	return 'Set group description to:\n'..deskripsi
end

local function get_description(msg, data)
  local data_cat = 'description'
  if not data[tostring(msg.to.id)][data_cat] then
    return 'No description available.'
	end
  local about = data[tostring(msg.to.id)][data_cat]
  return string.gsub(msg.to.print_name, "_", " ")..':\n\n'..about
end

local function set_rules(msg, data)
  if not is_mod(msg) then return moderators_only end
  local data_cat = 'rules'
	data[tostring(msg.to.id)][data_cat] = rules
	save_data(_config.moderation.data, data)

	return 'Set group rules to:\n'..rules
end

local function get_rules(msg, data)
  local data_cat = 'rules'
  if not data[tostring(msg.to.id)][data_cat] then
    return 'No rules available.'
	end
  local rules = data[tostring(msg.to.id)][data_cat]
  local rules = string.gsub(msg.to.print_name, '_', ' ')..' rules:\n\n'..rules
  return rules
end

-- dis/allow APIs bots to enter group. Spam prevention.
local function allow_api_bots(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_bot_lock = data[tostring(msg.to.id)]['settings']['lock_bots']
	if group_bot_lock == 'no' then
    return 'Bots are allowed to enter group.'
	else
    data[tostring(msg.to.id)]['settings']['lock_bots'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Group is open for bots.'
	end
end

local function disallow_api_bots(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_bot_lock = data[tostring(msg.to.id)]['settings']['lock_bots']
	if group_bot_lock == 'yes' then
    return 'Group is already locked from bots.'
	else
    data[tostring(msg.to.id)]['settings']['lock_bots'] = 'yes'
    save_data(_config.moderation.data, data)
    return 'Group is locked from bots.'
	end
end

-- lock/unlock group name. bot automatically change group name when locked
local function lock_group_name(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_name_set = data[tostring(msg.to.id)]['settings']['set_name']
  local group_name_lock = data[tostring(msg.to.id)]['settings']['lock_name']
	if group_name_lock == 'yes' then
    return 'Group name is already locked'
	else
    data[tostring(msg.to.id)]['settings']['lock_name'] = 'yes'
    save_data(_config.moderation.data, data)
    data[tostring(msg.to.id)]['settings']['set_name'] = string.gsub(msg.to.print_name, '_', ' ')
    save_data(_config.moderation.data, data)
	return 'Group name has been locked'
	end
end

local function unlock_group_name(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_name_set = data[tostring(msg.to.id)]['settings']['set_name']
  local group_name_lock = data[tostring(msg.to.id)]['settings']['lock_name']
	if group_name_lock == 'no' then
    return 'Group name is already unlocked'
	else
    data[tostring(msg.to.id)]['settings']['lock_name'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Group name has been unlocked'
	end
end

--lock/unlock group member. bot automatically kick new added user when locked
local function lock_group_member(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_member_lock = data[tostring(msg.to.id)]['settings']['lock_member']
	if group_member_lock == 'yes' then
    return 'Group members are already locked'
	else
    data[tostring(msg.to.id)]['settings']['lock_member'] = 'yes'
    save_data(_config.moderation.data, data)
	end
	return 'Group members has been locked'
end 

local function unlock_group_member(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_member_lock = data[tostring(msg.to.id)]['settings']['lock_member']
	if group_member_lock == 'no' then
    return 'Group members are not locked'
	else
    data[tostring(msg.to.id)]['settings']['lock_member'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Group members has been unlocked'
	end
end

--lock/unlock group photo. bot automatically keep group photo when locked
local function lock_group_photo(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_photo_lock = data[tostring(msg.to.id)]['settings']['lock_photo']
	if group_photo_lock == 'yes' then
    return 'Group photo is already locked'
	else
    data[tostring(msg.to.id)]['settings']['set_photo'] = 'waiting'
    save_data(_config.moderation.data, data)
	end
	return 'Please send me the group photo now'
end

local function unlock_group_photo(msg, data)
  if not is_mod(msg) then return moderators_only end
  local group_photo_lock = data[tostring(msg.to.id)]['settings']['lock_photo']
	if group_photo_lock == 'no' then
    return 'Group photo is not locked'
	else
    data[tostring(msg.to.id)]['settings']['lock_photo'] = 'no'
    save_data(_config.moderation.data, data)
    return 'Group photo has been unlocked'
	end
end

local function set_group_photo(msg, success, result)
  local data = load_data(_config.moderation.data)
  local receiver = get_receiver(msg)
  if success then
    local file = 'data/photos/chat_photo_'..msg.to.id..'.jpg'
    print('File downloaded to:', result)
    os.rename(result, file)
    print('File moved to:', file)
    chat_set_photo (receiver, file, ok_cb, false)
    data[tostring(msg.to.id)]['settings']['set_photo'] = file
    save_data(_config.moderation.data, data)
    data[tostring(msg.to.id)]['settings']['lock_photo'] = 'yes'
    save_data(_config.moderation.data, data)
    send_large_msg(receiver, 'Photo saved!', ok_cb, false)
  else
    print('Error downloading: '..msg.id)
    send_large_msg(receiver, 'Failed, please try again!', ok_cb, false)
  end
end

-- show group settings
local function show_group_settings(msg, data)
  if not is_mod(msg) then return moderators_only end
  local settings = data[tostring(msg.to.id)]['settings']
  if settings.lock_bots == 'yes' then
    lock_bots_state = 'ًں”’'
  elseif settings.lock_bots == 'no' then
    lock_bots_state = 'ًں”“'
  end
  if settings.lock_name == 'yes' then
    lock_name_state = 'ًں”’'
  elseif settings.lock_name == 'no' then
    lock_name_state = 'ًں”“'
  end
  if settings.lock_photo == 'yes' then
    lock_photo_state = 'ًں”’'
  elseif settings.lock_photo == 'no' then
    lock_photo_state = 'ًں”“'
  end
  if settings.lock_member == 'yes' then
    lock_member_state = 'ًں”’'
  elseif settings.lock_member == 'no' then
    lock_member_state = 'ًں”“'
  end
  if settings.anti_flood ~= 'no' then
    antiflood_state = 'ًں”’'
  elseif settings.anti_flood == 'no' then
    antiflood_state = 'ًں”“'
  end
  if settings.welcome ~= 'no' then
    greeting_state = 'ًں”’'
  elseif settings.welcome == 'no' then
    greeting_state = 'ًں”“'
  end
  if settings.sticker ~= 'ok' then
    sticker_state = 'ًں”’'
  elseif settings.sticker == 'ok' then
    sticker_state = 'ًں”“'
  end
  local text = 'Group settings:\n'
        ..'\n'..lock_bots_state..' Lock group from bot : '..settings.lock_bots
        ..'\n'..lock_name_state..' Lock group name : '..settings.lock_name
        ..'\n'..lock_photo_state..' Lock group photo : '..settings.lock_photo
        ..'\n'..lock_member_state..' Lock group member : '..settings.lock_member
        ..'\n'..antiflood_state..' Flood protection : '..settings.anti_flood
        ..'\n'..greeting_state..' Welcome message : '..settings.welcome
        ..'\n'..sticker_state..' Sticker policy : '..settings.sticker
  return text
end

-- media handler. needed by group_photo_lock
local function pre_process(msg)
  if not msg.text and msg.media then
    msg.text = '['..msg.media.type..']'
  end
  return msg
end

function run(msg, matches)

  if not is_chat_msg(msg) then
	    return "This is not a group chat."
	end

  local data = load_data(_config.moderation.data)
  local receiver = get_receiver(msg)

  -- create a group
  if matches[1] == 'mkgroup' and matches[2] then
    group_name = matches[2]
    return create_group(msg)
  end

  -- add a group to be moderated
  if matches[1] == 'addgroup' then
    return addgroup(msg)
  end

  -- remove group from moderation
  if matches[1] == 'remgroup' then
    return remgroup(msg)
  end

  if msg.media and is_chat_msg(msg) and is_momod(msg) then
    if msg.media.type == 'photo' and data[tostring(msg.to.id)] then
      if data[tostring(msg.to.id)]['settings']['set_photo'] == 'waiting' then
        load_photo(msg.id, set_group_photo, msg)
      end
    end
  end

  if data[tostring(msg.to.id)] then

    local settings = data[tostring(msg.to.id)]['settings']

    if matches[1] == 'setabout' and matches[2] then
      deskripsi = matches[2]
      return set_description(msg, data)
		end

		if matches[1] == 'about' then
      return get_description(msg, data)
		end

		if matches[1] == 'setrules' then
      rules = matches[2]
      return set_rules(msg, data)
		end

		if matches[1] == 'rules' then
      return get_rules(msg, data)
		end

    -- group link {get|set}
    if matches[1] == 'link' then
      if matches[2] == 'get' then
        if data[tostring(msg.to.id)]['link'] then
          local about = get_description(msg, data)
          local link = data[tostring(msg.to.id)]['link']
          return about.."\n\n"..link
        else
          return 'Invite link does not exist.\nTry !link set to generate it.'
        end
      end
      if matches[2] == 'set' and is_mod(msg) then
        msgr = export_chat_link(receiver, export_chat_link_callback, {data=data, msg=msg})
      end
	  end

		if matches[1] == 'group' then
      -- lock {bot|name|member|photo|sticker}
      if matches[2] == 'lock' then
        if matches[3] == 'bot' then
          return disallow_api_bots(msg, data)
        end
        if matches[3] == 'name' then
          return lock_group_name(msg, data)
        end
        if matches[3] == 'member' then
          return lock_group_member(msg, data)
        end
        if matches[3] == 'photo' then
          return lock_group_photo(msg, data)
        end
      -- unlock {bot|name|member|photo|sticker}
		  elseif matches[2] == 'unlock' then
        if matches[3] == 'bot' then
          return allow_api_bots(msg, data)
        end
        if matches[3] == 'name' then
          return unlock_group_name(msg, data)
        end
        if matches[3] == 'member' then
          return unlock_group_member(msg, data)
        end
        if matches[3] == 'photo' then
          return unlock_group_photo(msg, data)
        end
      -- view group settings
      elseif matches[2] == 'settings' then
        return show_group_settings(msg, data)
		  end
    end
    if matches[1] == 'sticker' then
      if matches[2] == 'warn' then
        if welcome_stat ~= 'warn' then
          data[tostring(msg.to.id)]['settings']['sticker'] = 'warn'
          save_data(_config.moderation.data, data)
        end
        return '[Alredy Enabled]\nSticker Sender will be warned first, then kicked for second Sticker.'
      end
      if matches[2] == 'kick' then
        if welcome_stat ~= 'kick' then
          data[tostring(msg.to.id)]['settings']['sticker'] = 'kick'
          save_data(_config.moderation.data, data)
        end
        return '[Already Enabled]Sticker Sender will be kicked!'
      end
      if matches[2] == 'ok' then
        if welcome_stat == 'ok' then
          return '[Already Disabled]Nothing Will Happend If Sticker Sent!'
        else
          data[tostring(msg.to.id)]['settings']['sticker'] = 'ok'
          save_data(_config.moderation.data, data)
          return 'Nothing Will Happend If Sticker Sent! '
        end
      end
    end

    -- if group name is renamed
    if matches[1] == 'chat_rename' then
      if not msg.service then
        return 'Are you trying to troll me?'
      end
      local group_name_set = settings.set_name
      local group_name_lock = settings.lock_name
      local to_rename = 'chat#id'..msg.to.id
      if group_name_lock == 'yes' then
        if group_name_set ~= tostring(msg.to.print_name) then
          rename_chat(to_rename, group_name_set, ok_cb, false)
        end
      elseif group_name_lock == 'no' then
        return nil
      end
		end

    -- set group name
		if matches[1] == 'setname' and is_mod(msg) then
      local new_name = string.gsub(matches[2], '_', ' ')
      data[tostring(msg.to.id)]['settings']['set_name'] = new_name
      save_data(_config.moderation.data, data)
      local group_name_set = data[tostring(msg.to.id)]['settings']['set_name']
      local to_rename = 'chat#id'..msg.to.id
      rename_chat(to_rename, group_name_set, ok_cb, false)
		end

    -- set group photo
		if matches[1] == 'setphoto' and is_mod(msg) then
      data[tostring(msg.to.id)]['settings']['set_photo'] = 'waiting'
      save_data(_config.moderation.data, data)
      return 'Please send me new group photo now'
		end

    -- if a user is added to group
		if matches[1] == 'chat_add_user' then
      if not msg.service then
        return 'Are you trying to troll me?'
      end
      local group_member_lock = settings.lock_member
      local group_bot_lock = settings.lock_bots
      local user = 'user#id'..msg.action.user.id
      if group_member_lock == 'yes' then
        chat_del_user(receiver, user, ok_cb, true)
      -- no APIs bot are allowed to enter chat group.
      elseif group_bot_lock == 'yes' and msg.action.user.flags == 4352 then
        chat_del_user(receiver, user, ok_cb, true)
      elseif group_bot_lock == 'no' or group_member_lock == 'no' then
        return nil
      end
    end

    -- if sticker is sent
    if msg.media and msg.media.caption == 'sticker.webp' and not is_momod(msg) then
      local user_id = msg.from.id
      local chat_id = msg.to.id
      local sticker_hash = 'mer_sticker:'..chat_id..':'..user_id
      local is_sticker_offender = redis:get(sticker_hash)
      if settings.sticker == 'warn' then
        if is_sticker_offender then
          chat_del_user(receiver, 'user#id'..user_id, ok_cb, true)
          redis:del(sticker_hash)
          return '[Warned Before]Kicked Because You Have Sent Stickers'
        elseif not is_sticker_offender then
          redis:set(sticker_hash, true)
          return ' Stop Sending Sticker.This Is A Warn Next Time You Will Kicked!'
        end
      elseif settings.sticker == 'kick' then
        chat_del_user(receiver, 'user#id'..user_id, ok_cb, true)
        return 'You Kicked Because You Have Sent Stickers??'
      elseif settings.sticker == 'ok' then
        return nil
      end
    end

    -- if group photo is deleted
		if matches[1] == 'chat_delete_photo' then
      if not msg.service then
        return 'Are you trying to troll me?'
      end
      local group_photo_lock = settings.lock_photo
      if group_photo_lock == 'yes' then
        chat_set_photo (receiver, settings.set_photo, ok_cb, false)
      elseif group_photo_lock == 'no' then
        return nil
      end
		end

    -- if group photo is changed
		if matches[1] == 'chat_change_photo' and msg.from.id ~= 0 then
      if not msg.service then
        return 'Are you trying to troll me?'
      end
      local group_photo_lock = settings.lock_photo
      if group_photo_lock == 'yes' then
        chat_set_photo (receiver, settings.set_photo, ok_cb, false)
      elseif group_photo_lock == 'no' then
        return nil
      end
    end

  end
end

return {
  description = 'Plugin to manage group chat.',
  usage = {
    admin = {
      '!mkgroup <group_name> : Make/create a new group.',
      '!addgroup : Add group to moderation list.',
      '!remgroup : Remove group from moderation list.'
    },
    moderator = {
      '!group <lock|unlock> bot : {Dis}allow APIs bots.',
      '!group <lock|unlock> member : Lock/unlock group member.',
      '!group <lock|unlock> name : Lock/unlock group name.',
      '!group <lock|unlock> photo : Lock/unlock group photo.',
      '!group settings : Show group settings.',
      '!link <set> : Generate/revoke invite link.',
      '!setabout <description> : Set group description.',
      '!setname <new_name> : Set group name.',
      '!setphoto : Set group photo.',
      '!setrules <rules> : Set group rules.',
      '!sticker warn : Sticker restriction, sender will be warned for the first violation.',
      '!sticker kick : Sticker restriction, sender will be kick.',
      '!sticker ok : Disable sticker restriction.'
    },
    user = {
      '!about : Read group description',
      '!rules : Read group rules',
      '!link <get> : Print invite link'
    },
  },
  patterns = {
    --"^!(about)$",
    --"^!(addgroup)$",
    "%[(audio)%]",
    "%[(document)%]",
    --"^!(group) (lock) (.*)$",
    --"^!(group) (settings)$",
    --"^!(group) (unlock) (.*)$",
    --"^!(link) (.*)$",
    --"^!(mkgroup) (.*)$",
    --"%[(photo)%]",
    --"^!(remgroup)$",
    --"^!(rules)$",
  --  "^!(setabout) (.*)$",
   -- "^!(setname) (.*)$",
    --"^!(setphoto)$",
  --"^!(setrules) (.*)$",
    "^[!/](sticker) (.*)$",
    "^!!tgservice (.+)$",
    "%[(video)%]"
  },
  run = run,
  pre_process = pre_process
}

end

--To Have This Update Lua-tg-c avaiable on tg folder
