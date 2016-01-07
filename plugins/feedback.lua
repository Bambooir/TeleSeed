do
 
local function run(msg, matches)
    if matches[1] then
        local fuse = '#Feedback\n\nId : #' .. msg.from.id .. '\n\nName: ' ..string.gsub(msg.from.print_name, "_", " ") ..'\n\nUsername: @' .. msg.from.username .. '\n\nFeedback: \n\n     ' .. matches[1] 
   	local chat = "chat#id"..84180057 --like : local chat = "chat#id"..12345678
  	local sends = send_large_msg(chat, fuse, ok_cb, false)
  	return 'Sent!'
    end
end
 return {
  description = "Feedback",
  usage = "!feedback message",
  patterns = {
  "^[!/][Ff]eedback (.*)$"
  },
  run = run
}
end

--[[do
local function run(msg, matches)
    print_name = string.gsub(msg.from.print_name, "_", " ")
    local fuse = 'تم تلقي رسالة جديدة #Feedback\n\nالايدي #: ' .. msg.from.id .. '\n\nالاسم : '..print_name ..'\n\nالمعرف : @' .. msg.from.username .. '\n\nالرسالة :\n     ' .. matches[1] 
    local chat = "chat#id"..84180057
    local sends = send_large_msg(chat, fuse, ok_cb, false)
        return 'تـــم ارســــــال رســــالتك بــنــجاح..'
end

return {
patterns = {
"^[!/][Ff]eedback (.*)$"
},
run = run
}
end]]
