do
-- Will leave the group if be added
local function run(msg)
    if msg.action.type == "chat_add_user" and msg.action.user.id == tonumber(our_id) then
       chat_del_user("chat#id"..msg.to.id, 'user#id'..our_id, ok_cb, false)
	end
end

return {
  patterns = {
	"^!!tgservice (.+)$",
  },
  run = run
}
end
