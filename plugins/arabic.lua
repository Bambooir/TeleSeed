local function run(msg)
    local hash = 'arabic:'..msg.to.id
    if redis:get(hash) then
        delete_msg(msg.id, ok_cb, false)
        if msg.to.type == 'chat' then
            send_msg('chat#id'..msg.to.id, lang_text(msg.to.id, 'noArabicT'), ok_cb, true)
            chat_del_user('chat#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, true)
        elseif msg.to.type == 'channel' then
            send_msg('channel#id'..msg.to.id, lang_text(msg.to.id, 'noArabicL'), ok_cb, true)
            channel_kick_user('channel#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, true)
        end
        return
    end
end   

return {
    patterns = {
        '([\216-\219][\128-\191])'
    },
    run = run
}