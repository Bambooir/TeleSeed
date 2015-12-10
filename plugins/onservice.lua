do
-- Will leave the group if be added
local function run(msg)
local bot_id = our_id -- your bot id
   -- like local bot_id = 1234567
    if msg.action.type == "chat_add_user" and msg.action.user.id == tonumber(bot_id) then
       send_large_msg("chat#id"..msg.to.id, 'this is not one of my groups.', ok_cb, false)
       chat_del_user("chat#id"..msg.to.id, 'user#id'..bot_id, ok_cb, false)
	end
end

return {
  patterns = {
	"^!!tgservice (.+)$",
  },
  run = run
}
end
