local function run(msg, matches)
    if matches[1] == "block" then
	local user_id = "user#id"..matches[2]
	block_user(user_id, ok_cb, false)
    end
    if matches[1] == "unblock" then
	local user_id = "user#id"..matches[2]
	unblock_user(user_id, ok_cb, false)
    end
end

return {
  patterns = {
    "^[!/](block) (.*)$",
    "^[!/](unblock) (.*)$"
  },
  run = run
}
