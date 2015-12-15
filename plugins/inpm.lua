do
local function chat_list(msg)
    local data = load_data(_config.moderation.data)
        local groups = 'groups'
        if not data[tostring(groups)] then
                return 'No groups at the moment'
        end
        local message = 'List of Groups:\n*Use /join (ID) to join*\n\n'
        for k,v in pairs(data[tostring(groups)]) do
                local settings = data[tostring(v)]['settings']
                if data[tostring(v)]['settings']['lock_member'] == 'yes' then
                   return
                end
                for m,n in pairs(settings) do
                        if m == 'set_name' then
                                name = n
                        end
                end
                message = message .. '👥 '.. name .. ' (ID: ' .. v .. ')\n\n'
        end
        local file = io.open("groups.txt", "w")
        file:write(message)
        file:flush()
        file:close()
        return message
       
end



function run(msg, matches)
  if msg.to.id == our_id or is_realm(msg) then
	 local data = load_data(_config.moderation.data)
    if matches[1] == 'join' and data[tostring(matches[2])] then
  		if is_banned(msg.from.id, matches[2]) then
	 		  return 'You are banned.'
	  	end
      if is_gbanned(msg.from.id) then
		    return 'You are globally banned.'
      end
      if data[tostring(matches[2])]['settings']['lock_member'] == 'yes' and not is_owner2(msg.from.id, matches[2]) then
        return 'Group is private.'
      end
      if data[tostring(matches[2])] == nil then
        return "I don't know that chat!"
      end
      local chat = "chat#id"..matches[2]
	      local user = "user#id"..msg.from.id
	      chat_add_user(chat, user, ok_cb, false)
              end
             -- SERVICE MESSAGE IN GROUP
            if msg.action and msg.action.type then
               local action = msg.action.type
             --Check if user from pm joins chat
            if action == "chat_add_user" and msg.action.to == matches[2] then
               print ("Added "..msg.from.print_name.."to chat ".."(ID:"..set_name..")")
               send_msg_large(user, "Added "..msg.from.print_name.."to chat ".."(ID:"..set_name..")")
            end
        end
     if matches[1] == 'chatlist'then
         return chat_list(msg)
       end
     end
  end
end


return {
    patterns = {
      "^[/!](chatlist)$",
      "^[/!](join) (.*)$",
      "^!!tgservice (chat_add_user)$"
    },
    run = run,
}

