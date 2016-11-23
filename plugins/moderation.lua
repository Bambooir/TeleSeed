--------------------------------------------------
--      ____  ____ _____                        --
--     |    \|  _ )_   _|___ ____   __  __      --
--     | |_  )  _ \ | |/ ·__|  _ \_|  \/  |     --
--     |____/|____/ |_|\____/\_____|_/\/\_|     --
--                                              --
--------------------------------------------------
--                                              --
--       Developers: @Josepdal & @MaSkAoS       --
--     Support: @Skneos,  @iicc1 & @serx666     --
--                                              --
--------------------------------------------------

do

local function index_gban(user_id)
  for k,v in pairs(_gbans.gbans_users) do
    if tonumber(user_id) == tonumber(v) then
      return k
    end
  end
  -- If not found
  return false
end

local function unmute_by_reply(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local name = msg.from.username
    local hash = 'muted:'..chat..':'..user
    redis:del(hash)
    if msg.to.type == 'chat' then
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'userUnmuted:1')..' '..name..' ('..user..') '..lang_text(chat, 'userUnmuted:2'), ok_cb,  true)
    elseif msg.to.type == 'channel' then
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'userUnmuted:1')..' '..name..' ('..user..') '..lang_text(chat, 'userUnmuted:2'), ok_cb,  true)
    end
end

local function mute_by_reply(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local name = msg.from.username
    local hash = 'muted:'..chat..':'..user
    redis:set(hash, true)
    if msg.to.type == 'chat' then
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'userMuted:1')..' '..name..' ('..user..') '..lang_text(chat, 'userMuted:2'), ok_cb,  true)
    elseif msg.to.type == 'channel' then
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'userMuted:1')..' '..name..' ('..user..') '..lang_text(chat, 'userMuted:2'), ok_cb,  true)
    end
end

local function mute_by_username(cb_extra, success, result)
    chat_type = cb_extra.chat_type
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    user_username = result.username
    local hash =  'muted:'..chat_id..':'..user_id
    redis:set(hash, true)
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..user_username..' ('..user_id..') '..lang_text(chat_id, 'userMuted:2'), ok_cb,  true)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..user_username..' ('..user_id..') '..lang_text(chat_id, 'userMuted:2'), ok_cb,  true)
    end
end

local function unmute_by_username(cb_extra, success, result)
    chat_type = cb_extra.chat_type
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    user_username = result.username
    local hash =  'muted:'..chat_id..':'..user_id
    redis:del(hash)
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..user_username..' ('..user_id..') '..lang_text(chat_id, 'userUnmuted:2'), ok_cb,  true)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..user_username..' ('..user_id..') '..lang_text(chat_id, 'userUnmuted:2'), ok_cb,  true)
    end
end

local function kick_user(user_id, chat_id)
    local chat = 'chat#id'..chat_id
    local user = 'user#id'..user_id
    local channel = 'channel#id'..chat_id
    if user_id == tostring(our_id) then
        print("I won't kick myself!")
    else
        chat_del_user(chat, user, ok_cb, true)
        channel_kick_user(channel, user, ok_cb, true)
    end
end

local function ban_user(user_id, chat_id)
    local chat = 'chat#id'..chat_id
    if user_id == tostring(our_id) then
        print('I won\'t kick myself!')
    else
        -- Save to redis
        local hash =  'banned:'..chat_id..':'..user_id
        redis:set(hash, true)
    end
end

local function unban_user(user_id, chat_id)
    local hash =  'banned:'..chat_id..':'..user_id
    redis:del(hash)
end

local function is_banned(user_id, chat_id)
    local hash =  'banned:'..chat_id..':'..user_id
    local banned = redis:get(hash)
    return banned or false
end

local function chat_kick(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local chat_type = msg.to.type
    if chat_type == 'chat' then
        chat_del_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'kickUser:1')..' '..user..' '..lang_text(chat, 'kickUser:2'), ok_cb,  true)
    elseif chat_type == 'channel' then
        channel_kick_user('channel#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'kickUser:1')..' '..user..' '..lang_text(chat, 'kickUser:2'), ok_cb,  true)
    end
end

local function chat_ban(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local hash =  'banned:'..chat..':'..user
    redis:set(hash, true)
    chat_del_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
    send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'banUser:1')..' '..user..' '..lang_text(chat, 'banUser:2'), ok_cb,  true)
end

local function gban_by_reply(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local hash = 'gban:'..user
    redis:set(hash, true)
    if not is_gbanned_table(msg.to.id) then
        table.insert(_gbans.gbans_users, tonumber(msg.to.id))
        print(msg.to.id..' added to _gbans table')
        save_gbans()
    end
    if msg.to.type == 'chat' then
        chat_del_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'gbanUser:1')..' '..user..' '..lang_text(chat, 'gbanUser:2'), ok_cb,  true)
    elseif msg.to.type == 'channel' then
        channel_kick_user('channel#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'gbanUser:1')..' '..user..' '..lang_text(chat, 'gbanUser:2'), ok_cb,  true)
    end
end

local function ungban_by_reply(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    local indexid = index_gban(user)
    local hash = 'gban:'..user
    redis:del(hash)
    if is_gbanned_table(user_id) then
        table.remove(_gbans.gbans_users, indexid)
        print(user_id..' removed from _gbans table')
        save_gbans()
    end
    if msg.to.type == 'chat' then
        chat_add_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'ungbanUser:1')..' '..user..' '..lang_text(chat, 'ungbanUser:2'), ok_cb,  true)
    elseif msg.to.type == 'channel' then
        channel_invite_user('channel#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'ungbanUser:1')..' '..user..' '..lang_text(chat, 'ungbanUser:2'), ok_cb,  true)
    end
end

local function add_by_reply(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    if msg.to.type == 'chat' then
        chat_add_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('chat#id'..chat, 'ℹ️ '..lang_text(chat, 'addUser:1')..' '..user..' '..lang_text(chat, 'addUser:2'), ok_cb,  true)
    elseif msg.to.type == 'channel' then
        channel_invite_user('channel#id'..chat, 'user#id'..user, ok_cb, false)
        send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'addUser:1')..' '..user..' '..lang_text(chat, 'addUser:3'), ok_cb,  true)
    end
end

local function channel_ban(extra, success, result)
    local msg = result
    msg = backward_msg_format(msg)
    local chat = msg.to.id
    local user = msg.from.id
    channel_kick_user('channel#id'..chat, 'user#id'..user, ok_cb, true)
    send_msg('channel#id'..chat, 'ℹ️ '..lang_text(chat, 'banUser:1')..' '..user..' '..lang_text(chat, 'banUser:2'), ok_cb,  true)
    ban_user(user, chat)
end

local function chat_unban(extra, success, result)
    result = backward_msg_format(result)
    local msg = result
    local chat = msg.to.id
    local user = msg.from.id
    unban_user(user, chat)
    chat_add_user('chat#id'..chat, 'user#id'..user, ok_cb, false)
    send_msg(chat, 'User '..user..' unbanned', ok_cb,  true)
end

local function channel_unban(extra, success, result)
    local msg = result
    local msg = backward_msg_format(msg)
    local chat = msg.to.id
    local user = msg.from.id
    unban_user(user, chat)
    channel_invite_user('channel#id'..chat, 'user#id'..user, ok_cb, true)
    send_msg('channel#id'..chat, 'User '..user..' unbanned', ok_cb,  true)
    
end

local function ban_by_username(cb_extra, success, result)
    chat_type = cb_extra.chat_type
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    user_username = result.username
    local hash =  'banned:'..chat_id..':'..user_id
    redis:set(hash, true)
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'banUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'banUser:2'), ok_cb, false)
        chat_del_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'banUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'banUser:2'), ok_cb, false)
        channel_kick_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    end
    ban_user(user_id, chat_id)
end

local function kick_by_username(cb_extra, success, result)
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    chat_type = cb_extra.chat_type
    user_username = result.username
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'kickUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'kickUser:2'), ok_cb, false)
        chat_del_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'kickUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'kickUser:2'), ok_cb, false)
        channel_kick_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    end
end

local function gban_by_username(cb_extra, success, result)
    local chat_id = cb_extra.chat_id
    local user_id = result.peer_id
    local user_username = result.username
    local chat_type = cb_extra.chat_type
    local hash =  'gban:'..user_id
    redis:set(hash, true)
    if not is_gbanned_table(user_id) then
        table.insert(_gbans.gbans_users, tonumber(user_id))
        print(user_id..' added to _gbans table')
        save_gbans()
    end
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'gbanUser:2'), ok_cb, false)
        chat_del_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'gbanUser:2'), ok_cb, false)
        channel_kick_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    end
end

local function ungban_by_username(cb_extra, success, result)
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    user_username = result.username
    chat_type = cb_extra.chat_type
    local indexid = index_gban(user_id)
    local hash =  'gban:'..user_id
    redis:del(hash)
    if is_gbanned_table(user_id) then
        table.remove(_gbans.gbans_users, indexid)
        print(user_id..' removed from _gbans table')
        save_gbans()
    end
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'ungbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'ungbanUser:2'), ok_cb, false)
        chat_add_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'ungbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'ungbanUser:2'), ok_cb, false)
        channel_invite_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    end
end

local function unban_by_username(cb_extra, success, result)
    chat_type = cb_extra.chat_type
    chat_id = cb_extra.chat_id
    user_id = result.peer_id
    user_username = result.username
    local hash =  'banned:'..chat_id..':'..user_id
    redis:del(hash)
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'unbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'unbanUser:2'), ok_cb, false)
        chat_add_user('chat#id'..chat_id, 'user#id'..user_id, callback, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'unbanUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'unbanUser:2'), ok_cb, false)
        channel_invite_user('channel#id'..chat_id, 'user#id'..user_id, callback, false)
    end
end

local function add_by_username(cb_extra, success, result)
    local chat_type = cb_extra.chat_type
    local chat_id = cb_extra.chat_id
    local user_id = result.peer_id
    local user_username = result.username
    print(chat_id)
    if chat_type == 'chat' then
        send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'addUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'addUser:2'), ok_cb, false)
        chat_add_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    elseif chat_type == 'channel' then
        send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'addUser:1')..' @'..user_username..' ('..user_id..') '..lang_text(chat_id, 'addUser:3'), ok_cb, false)
        channel_invite_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
    end
end

local function is_gbanned(user_id)
    local hash =  'gban:'..user_id
    local banned = redis:get(hash)
    return banned or false
end

local function pre_process(msg)
    --Checking mute
    local hash = 'muted:'..msg.to.id..':'..msg.from.id
    if redis:get(hash) then
        if msg.to.type == 'chat' then
            delete_msg(msg.id, ok_cb, true)
        elseif msg.to.type == 'channel' then
            delete_msg(msg.id, ok_cb, true)
        end
    end

    -- SERVICE MESSAGE
    if msg.action and msg.action.type then
        local action = msg.action.type
        -- Check if banned user joins chat
        if action == 'chat_add_user' or action == 'chat_add_user_link' then
            local user_id
            local hash = 'lockmember:'..msg.to.id
            if redis:get(hash) then
                if msg.action.link_issuer then
                    user_id = msg.from.id
                else
                    user_id = msg.action.user.id
                end
                kick_user(user_id, msg.to.id)
                delete_msg(msg.id, ok_cb, true)
            end
            if msg.action.link_issuer then
                user_id = msg.from.id
            else
                user_id = msg.action.user.id
            end
            print('Checking invited user '..user_id)
            local banned = is_banned(user_id, msg.to.id) or is_gbanned(user_id)
            if banned then
                print('User is banned!')
                kick_user(user_id, msg.to.id)
            end
        end
        -- No further checks
        return msg
    end

    -- BANNED USER TALKING
    if msg.to.type == 'chat' or msg.to.type == 'channel' then
        local user_id = msg.from.id
        local chat_id = msg.to.id
        local banned = is_banned(user_id, chat_id) or is_gbanned(user_id, msg.to.id)
        if banned then
            print('Banned user talking!')
            ban_user(user_id, chat_id)
            kick_user(user_id, chat_id)
            msg.text = ''
        end
        hash = 'antibot:'..msg.to.id
        if redis:get(hash) then
            if string.sub(msg.from.username, (string.len(msg.from.username)-2), string.len(msg.from.username)) == 'bot' then
                kick_user(user_id, chat_id)
            end
        end
    end
    return msg
end

local function run(msg, matches)
    chat_id =  msg.to.id
    if matches[1] == 'ban' then
        if permissions(msg.from.id, msg.to.id, "ban") then
            local chat_id = msg.to.id
            local chat_type = msg.to.type
            if msg.reply_id then
                if msg.to.type == 'chat' then
                    get_message(msg.reply_id, chat_ban, false)
                elseif msg.to.type == 'channel' then
                    get_message(msg.reply_id, channel_ban, {receiver=get_receiver(msg)})
                end
                return
            end
            if not is_id(matches[2]) then
                local member = string.gsub(matches[2], '@', '')
                resolve_username(member, ban_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                user_id = matches[2]
                if chat_type == 'chat' then
                    send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'banUser:1')..' '..user_id..' '..lang_text(chat, 'banUser:2'), ok_cb, false)
                    chat_del_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                elseif chat_type == 'channel' then
                    send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'banUser:1')..' '..user_id..' '..lang_text(chat, 'banUser:2'), ok_cb, false)
                    channel_kick_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                end
                ban_user(user_id, chat_id)
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'unban' then
        if permissions(msg.from.id, msg.to.id, "unban") then
            local chat_id = msg.to.id
            local chat_type = msg.to.type
            if msg.reply_id then
                if msg.to.type == 'chat' then
                    get_message(msg.reply_id, chat_unban, false)
                elseif msg.to.type == 'channel' then
                    get_message(msg.reply_id, channel_unban, false)
                end
                return
            end
            if not is_id(matches[2]) then
                local member = string.gsub(matches[2], '@', '')
                resolve_username(member, unban_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                local hash =  'banned:'..chat_id..':'..matches[2]
                redis:del(hash)
                if msg.to.type == 'chat' then
                    chat_add_user('chat#id'..chat_id, 'user#id'..matches[2], ok_cb, false)
                elseif msg.to.type == 'channel' then
                    channel_invite_user('channel#id'..chat_id, 'user#id'..matches[2], ok_cb, false)
                end
                return 'ℹ️ '..lang_text(chat_id, 'unbanUser:1')..' '..matches[2]..' '..lang_text(chat_id, 'unbanUser:2')
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'kick' then
        if permissions(msg.from.id, msg.to.id, "kick") then
            local chat_id = msg.to.id
            local chat_type = msg.to.type
            -- Using pattern #kick
            if msg.reply_id then
                get_message(msg.reply_id, chat_kick, false)
                return
            end
            if not is_id(matches[2]) then
                local member = string.gsub(matches[2], '@', '')
                resolve_username(member, kick_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                local user_id = matches[2]
                if msg.to.type == 'chat' then
                    send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'kickUser:1')..' '..user_id..' '..lang_text(chat_id, 'kickUser:2'), ok_cb, false)
                    chat_del_user('chat#id'..msg.to.id, 'user#id'..matches[2], ok_cb, false)
                elseif msg.to.type == 'channel' then
                    send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'kickUser:1')..' '..user_id..' '..lang_text(chat_id, 'kickUser:2'), ok_cb, false)
                    channel_kick_user('channel#id'..msg.to.id, 'user#id'..matches[2], ok_cb, false)
                end
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'gban' then
        if permissions(msg.from.id, msg.to.id, "gban") then
            chat_id = msg.to.id
            chat_type = msg.to.type
            if msg.reply_id then
                get_message(msg.reply_id, gban_by_reply, false)
                return
            end
            if not is_id(matches[2]) then
                local member = string.gsub(matches[2], '@', '')
                resolve_username(member, gban_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                local user_id = matches[2]
                local hash = 'gban:'..user_id
                redis:set(hash, true)
                if not is_gbanned_table(user_id) then
                    table.insert(_gbans.gbans_users, tonumber(user_id))
                    print(user_id..' added to _gbans table')
                    save_gbans()
                end
                if chat_type == 'chat' then
                    send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' '..user_id..' '..lang_text(chat_id, 'gbanUser:2'), ok_cb, false)
                    chat_del_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                elseif chat_type == 'channel' then
                    send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' '..user_id..' '..lang_text(chat_id, 'gbanUser:2'), ok_cb, false)
                    channel_kick_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                end
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'ungban' then
        if permissions(msg.from.id, msg.to.id, "ungban") then
        	chat_id = msg.to.id
        	chat_type = msg.to.type
            if msg.reply_id then
                get_message(msg.reply_id, ungban_by_reply, false)
                return
            end
            if not is_id(matches[2]) then
                local chat_type = msg.to.type
                local member = string.gsub(matches[2], '@', '')
                resolve_username(member, ungban_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                local user_id = matches[2]
                local hash = 'gban:'..user_id
                local indexid = index_gban(user_id)
                redis:del(hash)
                if is_gbanned_table(user_id) then
                    table.remove(_gbans.gbans_users, indexid)
                    print(user_id..' removed from _gbans table')
                    save_gbans()
                end
                if chat_type == 'chat' then
                    send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' '..user_id..' '..lang_text(chat_id, 'ungbanUser:2'), ok_cb, false)
                    chat_add_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                elseif chat_type == 'channel' then
                    send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'gbanUser:1')..' '..user_id..' '..lang_text(chat_id, 'ungbanUser:2'), ok_cb, false)
                    channel_invite_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                end
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'add' then
        if permissions(msg.from.id, msg.to.id, "add") then
            local chat_id = msg.to.id
            local chat_type = msg.to.type
            if msg.reply_id then
                get_message(msg.reply_id, add_by_reply, false)
                return
            end
            if not is_id(matches[2]) then
                local member = string.gsub(matches[2], '@', '')
                print(chat_id)
                resolve_username(member, add_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                return
            else
                local user_id = matches[2]
                if chat_type == 'chat' then
                    send_msg('chat#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'addUser:1')..' '..user_id..' '..lang_text(chat_id, 'addUser:2'), ok_cb, false)
                    chat_add_user('chat#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                elseif chat_type == 'channel' then
                    send_msg('channel#id'..chat_id, 'ℹ️ '..lang_text(chat_id, 'addUser:1')..' '..user_id..' '..lang_text(chat_id, 'addUser:3'), ok_cb, false)
                    channel_invite_user('channel#id'..chat_id, 'user#id'..user_id, ok_cb, false)
                end
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'mute' then
        if permissions(msg.from.id, msg.to.id, "mute") then
            if msg.reply_id then
                get_message(msg.reply_id, mute_by_reply, false)
                return
            end
            if matches[2] then
                if is_id(matches[2]) then
                    local hash = 'muted:'..msg.to.id..':'..matches[2]
                    redis:set(hash, true)
                    if msg.to.type == 'chat' then
                        send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..matches[2]..' '..lang_text(chat_id, 'userMuted:2'), ok_cb, true)
                    elseif msg.to.type == 'channel' then
                        send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(chat_id, 'userMuted:1')..' '..matches[2]..' '..lang_text(chat_id, 'userMuted:2'), ok_cb, true)
                    end
                    return
                else
                    local member = string.gsub(matches[2], '@', '')
                    local chat_id = msg.to.id
                    local chat_type = msg.to.type
                    resolve_username(member, mute_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                    return
                end
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'unmute' then
        if permissions(msg.from.id, msg.to.id, "unmute") then
            if msg.reply_id then
                get_message(msg.reply_id, unmute_by_reply, false)
                return
            end
            if matches[2] then
                if is_id(matches[2]) then
                    local hash = 'muted:'..msg.to.id..':'..matches[2]
                    redis:del(hash)
                    if msg.to.type == 'chat' then
                        send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(chat_id, 'userUnmuted:1')..' '..matches[2]..' '..lang_text(chat_id, 'userUnmuted:2'), ok_cb, true)
                    elseif msg.to.type == 'channel' then
                        send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(chat_id, 'userUnmuted:1')..' '..matches[2]..' '..lang_text(chat_id, 'userUnmuted:2'), ok_cb, true)
                    end
                    return
                else
                    local member = string.gsub(matches[2], '@', '')
                    local chat_id = msg.to.id
                    local chat_type = msg.to.type
                    resolve_username(member, unmute_by_username, {chat_id=chat_id, member=member, chat_type=chat_type})
                    return
                end
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'kickme' then
        local hash = 'kickme:'..msg.to.id
        if redis:get(hash) then
            if msg.to.type == 'chat' then
                send_msg('chat#id'..msg.to.id, '👋🏽 '..lang_text(chat_id, 'kickmeBye')..' @'..msg.from.username..' ('..msg.from.id..').', ok_cb, true)
                chat_del_user('chat#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, false)
            elseif msg.to.type == 'channel' then
                send_msg('channel#id'..msg.to.id, '👋🏽 '..lang_text(chat_id, 'kickmeBye')..' @'..msg.from.username..' ('..msg.from.id..').', ok_cb, true)
                channel_kick_user('channel#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, false)
            end
        end  
    end
end

return {
    patterns = {
        "^[!/#](ban) (.*)$",
        "^[!/#](ban)$",
        "^[!/#](unban) (.*)$",
        "^[!/#](unban)$",
        "^[!/#](kick) (.*)$",
        "^[!/#](kick)$",
        "^[!/#](kickme)$",
        "^[!/#](add) (.*)$",
        "^[!/#](add)$",
        "^[!/#](gban) (.*)$",
        "^[!/#](gban)$",
        "^[!/#](ungban) (.*)$",
        "^[!/#](ungban)$",
        '^[!/#](mute) (.*)$',
        '^[!/#](mute)$',
        '^[!/#](unmute) (.*)$',
        '^[!/#](unmute)$',
        "^!!tgservice (.*)$"
    },
    run = run,
    pre_process = pre_process
}
end
