do
-- Will leave unmoderated groups if added
local function run(msg, matches)
local bot_id = our_id -- your bot id - like bot_id = 1234567

    if matches[1] == 'leave' and is_admin(msg) then
       kick_user_any(bot_id, msg.to.id) -- kick me (bot)
    end
    -- SERVICE MESSAGE
    if msg.service and msg.action.type == "chat_add_user" and msg.action.user.id == tonumber(our_id) then
       if is_admin(msg) then -- if added by an admin dont leave
          return  --Do nothing 
        else 
        local data = load_data(_config.moderation.data) -- load group mod data
            if not data[tostring(msg.to.id)] then -- check group and if group is not our group
               print ("Not a added group.")
               kick_user_any(bot_id, msg.to.id) -- kick me (bot)
            end
        end
     end
end


return {
  patterns = {
       "^[!/](leave)$",
       "^!!tgservice (.+)$",
  },
  run = run
}
end
