do
local function run(msg, matches, callback, extra)
local data = load_data(_config.moderation.data)
local rules = data[tostring(msg.to.id)]['rules']
local about = data[tostring(msg.to.id)]['description']
local hash = 'group:'..msg.to.id
local group_welcome = redis:hget(hash,'welcome')
if matches[1] == 'حذف ولکام' and not matches[2] and is_owner(msg) then 
    
   redis:hdel(hash,'welcome')
        return 'متن خوش آمد گویی پاک شد 🗑'
end

local url , res = http.request('http://api.gpmod.ir/time/')
if res ~= 200 then return "No connection" end
local jdat = json:decode(url)

if matches[1] == 'تنظیم ولکام' and is_owner(msg) then
redis:hset(hash,'welcome',matches[2])
        return 'متن خوش آمد گویی گروه تنظیم شد به : ✋\n'..matches[2]
end

if matches[1] == 'chat_add_user' or 'chat_add_user_link' or 'channel_invite' and msg.service then
group_welcome = string.gsub(group_welcome, '{gpname}', msg.to.title)
group_welcome = string.gsub(group_welcome, '{firstname}', ""..(msg.action.user.first_name or '').."")
 group_welcome = string.gsub(group_welcome, '{lastname}', ""..(msg.action.user.last_name or '').."")
  group_welcome = string.gsub(group_welcome, '{username}', "@"..(msg.action.user.username or '').."")
  group_welcome = string.gsub(group_welcome, '{fatime}', ""..(jdat.FAtime).."")
  group_welcome = string.gsub(group_welcome, '{entime}', ""..(jdat.ENtime).."")
  group_welcome = string.gsub(group_welcome, '{fadate}', ""..(jdat.FAdate).."")
  group_welcome = string.gsub(group_welcome, '{endate}', ""..(jdat.ENdate).."")
  group_welcome = string.gsub(group_welcome, '{rules}', ""..(rules or '').."")
  group_welcome = string.gsub(group_welcome, '{about}', ""..(about or '').."")




group_welcome = string.gsub(group_welcome, '{نام گروه}', msg.to.title)
group_welcome = string.gsub(group_welcome, '{نام اول}', ""..(msg.action.user.first_name or '').."")
 group_welcome = string.gsub(group_welcome, '{نام آخر}', ""..(msg.action.user.last_name or '').."")
  group_welcome = string.gsub(group_welcome, '{نام کاربری}', "@"..(msg.action.user.username or '').."")
  group_welcome = string.gsub(group_welcome, '{ساعت فارسی}', ""..(jdat.FAtime).."")
  group_welcome = string.gsub(group_welcome, '{ساعت انگلیسی}', ""..(jdat.ENtime).."")
  group_welcome = string.gsub(group_welcome, '{تاریخ فارسی}', ""..(jdat.FAdate).."")
  group_welcome = string.gsub(group_welcome, '{تاریخ انگلیسی}', ""..(jdat.ENdate).."")

 end
return group_welcome
end
return {
  patterns = {
  "^(تنظیم ولکام) +(.*)$",
  "^(حذف ولکام)$",
  "^!!tgservice (chat_add_user)$",
  "^!!tgservice (invite)$",
  "^!!tgservice (chat_add_user_link)$",
  },
  run = run
}
end
