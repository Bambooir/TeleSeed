local function run(msg, matches)
  local text = matches[1]
  local b = 1

  while b ~= 0 do
    text = text:trim()
    text,b = text:gsub('^!+','')
  end
  local file = io.open("./data/echo"..msg.from.id..matches[2], "w")
  file:write(text)
  file:flush()
  file:close()
  send_document("chat#id"..msg.to.id,"./data/echo"..msg.from.id..matches[2], ok_cb, false)
end

return {
  description = "Simplest plugin ever!",
  usage = "!echo [whatever]: echoes the msg",
  patterns = {
    "^!echo +(.+) (.*)$"
  },
  run = run
}
