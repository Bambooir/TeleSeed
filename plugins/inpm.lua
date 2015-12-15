do
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
      if not data[tostring(matches[2])] then
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
     end
  end
end
return {
    patterns = {
      "^[/!](join) (.*)$",
      "^!!tgservice (chat_add_user)$"
    },
    run = run,
}

