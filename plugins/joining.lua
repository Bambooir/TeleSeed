local function run(msg, matches)
    if matches[1] == "support" then
 local user = "user#id"..msg.from.id
 local fachat = "chat#id"..107249662 
 chat_add_user(fachat, user, ok_cb, false)
    end
    if matches[1] == "development" then
 local user = "user#id"..msg.from.id
 local devchat = "chat#id"..100568698 
 chat_add_user(devchat, user, ok_cb, false)
    end
end

return {
  patterns = {
    "^[!/]([Ss]upport)$",
    "^[!/]([dD]evelopment)$"
  },
  run = run
}
