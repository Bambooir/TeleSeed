do
function run(msg, matches)
local reply_id = msg['id']
local url = "http://8pic.ir/images/1wushjmzxxcc25e6w9v8.jpg"
local receiver = get_receiver(msg)
local file = download_to_file(url)
local info = '🔵 #نام: '..msg.from.first_name..'\n'
..'🔴 #ایدی : '..msg.from.id..'\n'
..'🔵 #یوزرنیم @'..msg.from.username..'\n'
..'🔴 #ایدی گروه: '..msg.to.id..'\n'
..'🔵 #نام گروه: '..msg.to.title..'\n'
..'🔴 #کانال ما :  @Xx_FUNKADHE_xX '
if matches[1] == 'id' then
send_photo2(receiver, file, info, rmtmp_cb, cb_extra)
end
end
return {
patterns = {
    "^id"
},
run = run
}

end
