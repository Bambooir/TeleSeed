lock_group_fosh(msg, data, target)
      end
    if matches[2] == 'leave' then
       savelog(msg.to.id, name_log.." ["..msg.from.id.."] unlocked leaving ")
       return unlock_group_leave(msg, data, target)
     end
      if matches[2] == 'join' then
        savelog(msg.to.id, name_log.." ["..msg.from.id.."] unlocked joining link ")
        return unlock_group_join(msg, data, target)
      end
   end
    if matches[1] == 'settings' then
      local target = msg.to.id
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] requested group settings ")
      return show_group_settingsmod(msg, data, target)
    end	

  if matches[1] == 'public' then
    local target = msg.to.id
    if matches[2] == 'yes' then
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] set group to: public")
      return set_public_membermod(msg, data, target)
    end
    if matches[2] == 'no' then
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] set group to: not public")
      return unset_public_membermod(msg, data, target)
    end
  end

    if matches[1] == 'newlink' and not is_realm(msg) then
      if not is_momod(msg) then
        return "For moderators only!"
      end
      local function callback (extra , success, result)
        local receiver = 'chat#'..msg.to.id
        if success == 0 then
           return send_large_msg(receiver, '*Error: Invite link failed* \nReason: Not creator.')
        end
        send_large_msg(receiver, "Created a new link")
        data[tostring(msg.to.id)]['settings']['set_link'] = result
        save_data(_config.moderation.data, data)
      end
      local receiver = 'chat#'..msg.to.id
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] revoked group link ")
      return export_chat_link(receiver, callback, true)
    end
    if matches[1] == 'link' then
      if not is_momod(msg) then
        return "For moderators only!"
      end
      local group_link = data[tostring(msg.to.id)]['settings']['set_link']
      if not group_link then 
        return "Create a link using /newlink first !"
      end
       savelog(msg.to.id, name_log.." ["..msg.from.id.."] requested group link ["..group_link.."]")
      return "Group link:\n"..group_link
    end
    if matches[1] == 'setowner' and matches[2] then
      if not is_owner(msg) then
        return "For owner only!"
      end
      data[tostring(msg.to.id)]['set_owner'] = matches[2]
      save_data(_config.moderation.data, data)
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] set ["..matches[2].."] as owner")
      local text = matches[2].." added as owner"
      return text
    end
    if matches[1] == 'setowner' and not matches[2] then
      if not is_owner(msg) then
        return "only for the owner!"
      end
      if type(msg.reply_id)~="nil" then
          msgr = get_message(msg.reply_id, setowner_by_reply, false)
      end
    end
    if matches[1] == 'owner' then
      local group_owner = data[tostring(msg.to.id)]['set_owner']
      if not group_owner then 
        return "no owner,ask admins in support groups to set owner for your group"
      end
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] used /owner")
      return "Group owner is ["..group_owner..']'
    end
    if matches[1] == 'setgpowner' then
      local receiver = "chat#id"..matches[2]
      if not is_admin(msg) then
        return "For admins only!"
      end
      data[tostring(matches[2])]['set_owner'] = matches[3]
      save_data(_config.moderation.data, data)
      local text = matches[3].." added as owner"
      send_large_msg(receiver, text)
      return
    end
    if matches[1] == 'setflood' then 
      if not is_momod(msg) then
        return "For moderators only!"
      end
      if tonumber(matches[2]) < 5 or tonumber(matches[2]) > 20 then
        return "Wrong number,range is [5-20]"
      end
      local flood_max = matches[2]
      data[tostring(msg.to.id)]['settings']['flood_msg_max'] = flood_max
      save_data(_config.moderation.data, data)
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] set flood to ["..matches[2].."]")
      return 'Group flood has been set to '..matches[2]
    end
    if matches[1] == 'clean' then
      if not is_owner(msg) then
        return "Only owner can clean"
      end
      if matches[2] == 'member' then
        if not is_owner(msg) then
          return "Only admins can clean members"
        end
        local receiver = get_receiver(msg)
        chat_info(receiver, cleanmember, {receiver=receiver})
      end
      if matches[2] == 'modlist' then
        if next(data[tostring(msg.to.id)]['moderators']) == nil then --fix way
          return 'No moderator in this group.'
        end
        local message = '\nList of moderators for ' .. string.gsub(msg.to.print_name, '_', ' ') .. ':\n'
        for k,v in pairs(data[tostring(msg.to.id)]['moderators']) do
          data[tostring(msg.to.id)]['moderators'][tostring(k)] = nil
          save_data(_config.moderation.data, data)
        end
        savelog(msg.to.id, name_log.." ["..msg.from.id.."] cleaned modlist")
      end
      if matches[2] == 'rules' then 
        local data_cat = 'rules'
        data[tostring(msg.to.id)][data_cat] = nil
        save_data(_config.moderation.data, data)
        savelog(msg.to.id, name_log.." ["..msg.from.id.."] cleaned rules")
      end
      if matches[2] == 'about' then 
        local data_cat = 'description'
        data[tostring(msg.to.id)][data_cat] = nil
        save_data(_config.moderation.data, data)
        savelog(msg.to.id, name_log.." ["..msg.from.id.."] cleaned about")
      end     
    end
    if matches[1] == 'kill' and matches[2] == 'chat' then
      if not is_admin(msg) then
          return nil
      end
      if not is_realm(msg) then
          local receiver = get_receiver(msg)
          return modrem(msg),
          print("Closing Group..."),
          chat_info(receiver, killchat, {receiver=receiver})
      else
          return 'This is a realm'
      end
   end
    if matches[1] == 'kill' and matches[2] == 'realm' then
     if not is_admin(msg) then
         return nil
     end
     if not is_group(msg) then
        local receiver = get_receiver(msg)
        return realmrem(msg),
        print("Closing Realm..."),
        chat_info(receiver, killrealm, {receiver=receiver})
     else
        return 'This is a group'
     end
   end
    if matches[1] == 'help' then
      if not is_momod(msg) or is_realm(msg) then
        return
      end
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] Used /help")
      return help()
    end
    if matches[1] == 'info' and is_momod(msg) then 
      local cbres_extra = {
        chatid = msg.to.id
      }
      local username = matches[2]
      local username = username:gsub("@","")
      savelog(msg.to.id, name_log.." ["..msg.from.id.."] Used /res "..username)
      return res_user(username,  callbackres, cbres_extra)
    end
    if matches[1] == 'kickinactive' then
      --send_large_msg('chat#id'..msg.to.id, 'I\'m in matches[1]')
	    if not is_momod(msg) then
	      return 'Only a moderator can kick inactive users'
	    end
	    local num = 1
	    if matches[2] then
	        num = matches[2]
	    end
	    local chat_id = msg.to.id
	    local receiver = get_receiver(msg)
      return kick_inactive(chat_id, num, receiver)
    end
  end 
end

return {
  patterns = {
  "^[!/.]([Aa]dd)$",
  "^[!/.]([Aa]dd) (realm)$",
  "^[!/.]([Rr]em)$",
  "^[!/.]([Rr]em) (realm)$",
  "^[!/.]([Rr]ules)$",
  "^[!/.]([Aa]bout)$",
  "^[!/.]([Ss]etname) (.*)$",
  "^[!/.]([Ss]etphoto)$",
  "^[!/.]([Pp]romote) (.*)$",
  "^[!/.]([Pp]romote)",
  "^[!/.]([Hh]elp)$",
  "^[!/.]([Cc]lean) (.*)$",
  "^[!/.]([Kk]ill) (chat)$",
  "^[!/.]([Kk]ill) (realm)$",
  "^[!/.]([Dd]emote) (.*)$",
  "^[!/.]([Dd]emote)",
  "^[!/.]([Ss]et) ([^%s]+) (.*)$",
  "^[!/.]([Ss]ilent) (.*)$",
  "^[!/.]([Ll]ock) (.*)$",
  "^[!/.]([Ss]etowner) (%d+)$",
  "^[!/.]([Ss]etowner)",
  "^[!/.]([Oo]wner)$",
  "^[!/.]([Ii]nfo) (.*)$",
  "^[!/.]([Ss]etgpowner) (%d+) (%d+)$",-- (group id) (owner id)
  "^[!/.]([Uu]nlock) (.*)$",
  "^[!/.]([Ss]etflood) (%d+)$",
  "^[!/.]([Ss]ettings)$",
  "^[!/.]([Pp]ublic) (.*)$",
  "^[!/.]([Mm]odlist)$",
  "^[!/.]([Nn]ewlink)$",
  "^[!/.]([Ll]ink)$",
  "^[!/.]([Kk]ickinactive)$",
  "^[!/.]([Kk]ickinactive) (%d+)$",
  "^([Aa]dd)$",
  "^([Aa]dd) (realm)$",
  "^([Rr]em)$",
  "^([Rr]em) (realm)$",
  "^([Rr]ules)$",
  "^([Aa]bout)$",
  "^([Ss]etname) (.*)$",
  "^([Ss]etphoto)$",
  "^([Pp]romote) (.*)$",
  "^([Pp]romote)",
  "^([Hh]elp)$",
  "^([Cc]lean) (.*)$",
  "^([Kk]ill) (chat)$",
  "^([Kk]ill) (realm)$",
  "^([Dd]emote) (.*)$",
  "^([Dd]emote)",
  "^([Ss]et) ([^%s]+) (.*)$",
  "^([Ss]ilent) (.*)$",
  "^([Ll]ock) (.*)$",
  "^([Ww]elcome) (.*)$",
  "^([Ss]etowner) (%d+)$",
  "^([Ss]etowner)",
  "^([Oo]wner)$",
 -- "^([Ii]nfo) (.*)$",
  "^([Ss]etgpowner) (%d+) (%d+)$",-- (group id) (owner id)
  "^([Uu]nlock) (.*)$",
  "^([Ss]etflood) (%d+)$",
  "^([Ss]ettings)$",
  "^([Pp]ublic) (.*)$",
  "^([Mm]odlist)$",
  "^([Nn]ewlink)$",
  "^([Ll]ink)$",
  "^([Kk]ickinactive)$",
  "^([Kk]ickinactive) (%d+)$",
  "%[(photo)%]",
  "^!!tgservice (.+)$",
  },
  run = run
}
end

