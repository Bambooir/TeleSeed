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
--    #creategroup by @lamjavid &  @Josepdal	--
--												--
--------------------------------------------------

do

local function create_group(msg, group_name)
    local group_creator = msg.from.print_name
    create_group_chat(group_creator, group_name, ok_cb, false)
    return 'ℹ️ '..lang_text(msg.to.id, 'createGroup:1')..' "'..string.gsub(group_name, '_', ' ')..'" '..lang_text(msg.to.id, 'createGroup:2')
end

local function remove_message(extra, success, result)
    msg = backward_msg_format(result)
    delete_msg(msg.id, ok_cb, false)
end

local function set_group_photo(msg, success, result)
    local receiver = get_receiver(msg)
    if success then
        local file = 'data/photos/chat_photo_'..msg.to.id..'.jpg'
        print('File downloaded to:', result)
        os.rename(result, file)
        print('File moved to:', file)
        if msg.to.type == 'channel' then
        	channel_set_photo (receiver, file, ok_cb, false)
	elseif msg.to.type == 'chat' then
		chat_set_photo(receiver, file, ok_cb, false)
	end
        return 'ℹ️ '..lang_text(msg.to.id, 'photoSaved')
    else
        print('Error downloading: '..msg.id)
        return 'ℹ️ '..lang_text(msg.to.id, 'photoSaved')
    end
end

local function pre_process(msg)
    local hash = 'flood:max:'..msg.to.id
    if not redis:get(hash) then
        floodMax = 5
    else
        floodMax = tonumber(redis:get(hash))
    end

    local hash = 'flood:time:'..msg.to.id
    if not redis:get(hash) then
        floodTime = 3
    else
        floodTime = tonumber(redis:get(hash))
    end
    if not permissions(msg.from.id, msg.to.id, "pre_process") then
        --Checking flood
        local hashse = 'anti-flood:'..msg.to.id
        if not redis:get(hashse) then
            print('anti-flood enabled')
            -- Check flood
            if msg.from.type == 'user' then
                if not permissions(msg.from.id, msg.to.id, "no_flood_ban") then
                    -- Increase the number of messages from the user on the chat
                    local hash = 'flood:'..msg.from.id..':'..msg.to.id..':msg-num'
                    local msgs = tonumber(redis:get(hash) or 0)
                    if msgs > floodMax then
                        local receiver = get_receiver(msg)
                        local user = msg.from.id
                        local chat = msg.to.id
                        local channel = msg.to.id
                        local bhash = 'banned:'..msg.to.id..':'..msg.from.id
                        redis:set(bhash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, lang_text(chat, 'user')..' @'..msg.from.username..' ('..msg.from.id..') '..lang_text(chat, 'isFlooding'), ok_cb, true)
                            chat_del_user('chat#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, true)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, lang_text(chat, 'user')..' @'..msg.from.username..' ('..msg.from.id..') '..lang_text(chat, 'isFlooding'), ok_cb, true)
                            channel_kick_user('channel#id'..msg.to.id, 'user#id'..msg.from.id, ok_cb, true)
                        end
                    end
                    redis:setex(hash, floodTime, msgs+1)
                end
            end
        end

        if msg.action and msg.action.type then
            local action = msg.action.type
            if action == 'chat_add_user' or action == 'chat_add_user_link' then
            	hash = 'antibot:'..msg.to.id
            	if redis:get(hash) then
	                if string.sub(msg.action.user.username:lower(), -3) == 'bot' then
	                    if msg.to.type == 'chat' then
	                        chat_del_user('chat#id'..msg.to.id, 'user#id'..msg.action.user.id, ok_cb, true)
	                    elseif msg.to.type == 'channel' then
	                        channel_kick_user('channel#id'..msg.to.id, 'user#id'..msg.action.user.id, ok_cb, true)
	                    end
	                end
	            end
            end
        end

        --Checking stickers
        if not msg.media then
            webp = 'nothing'
        else
            webp = msg.media.caption
        end
        if webp == 'sticker.webp' then
            hash = 'stickers:'..msg.to.id
            if redis:get(hash) then
                delete_msg(msg.id, ok_cb, false)
            end
        end
        if not msg.media then
            mp4 = 'nothing'
        else
            if msg.media.type == 'document' then
                mp4 = msg.media.caption or 'audio'
            end
        end
	--Checking tgservices
        hash = 'tgservices:'..msg.to.id
        if redis:get(hash) then
		local action = msg.action.type
		if action == 'chat_add_user' or action == 'chat_add_user_link' or action == 'chat_del_user' then
		    delete_msg(msg.id, ok_cb, false)
		end
	end
        --Checking GIFs and MP4 files
        if mp4 == 'giphy.mp4' then
            hash = 'gifs:'..msg.to.id
            if redis:get(hash) then
                delete_msg(msg.id, ok_cb, false)
            end
        else
            if msg.media then
                if msg.media.type == 'document' then
                    gifytpe = string.find(mp4, 'gif.mp4') or 'audio'
                    if gifytpe == 'audio' then
                        hash = 'audio:'..msg.to.id
                        if redis:get(hash) then
                            delete_msg(msg.id, ok_cb, false)
                        end
                    else
                        hash = 'gifs:'..msg.to.id
                        if redis:get(hash) then
                            delete_msg(msg.id, ok_cb, false)
                        end
                    end
                end
            end
        end
        --Checking photos
        if msg.media then
            if msg.media.type == 'photo' then
                local hash = 'photo:'..msg.to.id
                if redis:get(hash) then
                    delete_msg(msg.id, ok_cb, false)
                end
            end
        end

        --Checking muteall
        local hash = 'muteall:'..msg.to.id
        if redis:get(hash) then
            delete_msg(msg.id, ok_cb, false)
        end
    else
        if msg.media then
            if msg.media.type == 'photo' then
                local hash = 'setphoto:'..msg.to.id..':'..msg.from.id
                if redis:get(hash) then
                    redis:del(hash)
                    load_photo(msg.id, set_group_photo, msg)
                    print('setphoto')
                    delete_msg(msg.id, ok_cb, false)
                end
            end
        end
    end
    return msg
end

local function run(msg, matches)
    if matches[1] == 'settings' then
        if permissions(msg.from.id, msg.to.id, "settings") then
            if matches[2] ~= nil then
                if matches[2] == 'stickers' then
                    if matches[3] == 'enable' then
                        hash = 'stickers:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'stickersT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'stickersL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'stickers:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noStickersT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noStickersL'), ok_cb, false)
                        end
                    end
                    return
		elseif matches[2] == 'tgservices' then
                     if matches[3] == 'enable' then
                     hash = 'tgservices:'..msg.to.id
                     redis:del(hash)
                       	if msg.to.type == 'chat' then
              		     send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'tgservicesT'), ok_cb, false)
                             elseif msg.to.type == 'channel' then
                       		send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'tgservicesL'), ok_cb, false)
                	     end
                    elseif matches[3] == 'disable' then
                       	hash = 'tgservices:'..msg.to.id
                	redis:set(hash, true)
                	if msg.to.type == 'chat' then
                       	    send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noTgservicesT'), ok_cb, false)
                       	elseif msg.to.type == 'channel' then
                       	   send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noTgservicesL'), ok_cb, false)
                	end
                    end
                    return
                elseif matches[2] == 'gifs' then
                    if matches[3] == 'enable' then
                        hash = 'gifs:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'gifsT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'gifsL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'gifs:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noGifsT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noGifsL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'links' then
                    if matches[3] == 'enable' then
                        hash = 'links:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'LinksT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'LinksL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'links:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noLinksT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noLinksL'), ok_cb, false)
                    end
            end
            return
                elseif matches[2] == 'photos' then
                    if matches[3] == 'enable' then
                        hash = 'photo:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'photosT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'photosL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'photo:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noPhotosT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noPhotosL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'bots' then
                    if matches[3] == 'enable' then
                        hash = 'antibot:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'botsT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'botsL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'antibot:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noBotsT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noBotsL'), ok_cb, false)
                        end
                    end
                    return
                 elseif matches[2] == 'arabic' then
                    if matches[3] == 'enable' then
                        hash = 'arabic:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'arabicT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'arabicL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'arabic:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noArabicT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noArabicL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'audios' then
                    if matches[3] == 'enable' then
                        hash = 'audio:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'audiosT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'audiosL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'audio:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noAudiosT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noAudiosL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'kickme' then
                    if matches[3] == 'enable' then
                        hash = 'kickme:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'kickmeT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'kickmeL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'kickme:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noKickmeT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noKickmeL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'flood' then
                    if matches[3] == 'enable' then
                        hash = 'anti-flood:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        hash = 'anti-flood:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noFloodT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noFloodL'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'spam' then
                    if matches[3] == 'enable' then
                        local hash = 'spam:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'allowedSpamT'), ok_cb, true)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'allowedSpamL'), ok_cb, true)
                        end
                    elseif matches[3] == 'disable' then
                        local hash = 'spam:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notAllowedSpamT'), ok_cb, true)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notAllowedSpamL'), ok_cb, true)
                        end
                    end
                elseif matches[2] == 'lockmember' then
                    hash = 'lockmember:'..msg.to.id
                    if matches[3] == 'enable' then
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'lockMembersT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'lockMembersL'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notLockMembersT'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notLockMembersT'), ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'floodtime' then
                    if not matches[3] then
                    else
                        hash = 'flood:time:'..msg.to.id
                        redis:set(hash, matches[3])
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodTime')..matches[3], ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodTime')..matches[3], ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'maxflood' then
                    if not matches[3] then
                    else
                        hash = 'flood:max:'..msg.to.id
                        redis:set(hash, matches[3])
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodMax')..matches[3], ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'floodMax')..matches[3], ok_cb, false)
                        end
                    end
                    return
                elseif matches[2] == 'setname' then
                    if matches[3] == 'enable' then
                        local hash = 'name:enabled:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'chatRename'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'channelRename'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        local hash = 'name:enabled:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notChatRename'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notChannelRename'), ok_cb, false)
                        end
                    end
                    elseif matches[2] == 'setphoto' then
                    if matches[3] == 'enable' then
                        local hash = 'setphoto:'..msg.to.id
                        redis:set(hash, true)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'chatSetphoto'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'channelSetphoto'), ok_cb, false)
                        end
                    elseif matches[3] == 'disable' then
                        local hash = 'setphoto:'..msg.to.id
                        redis:del(hash)
                        if msg.to.type == 'chat' then
                            send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notChatSetphoto'), ok_cb, false)
                        elseif msg.to.type == 'channel' then
                            send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'notChannelSetphoto'), ok_cb, false)
                        end
                    end
                end
            else
                if msg.to.type == 'chat' then
                    text = '⚙ '..lang_text(msg.to.id, 'gSettings')..':\n'
                elseif msg.to.type == 'channel' then
                    text = '⚙ '..lang_text(msg.to.id, 'sSettings')..':\n'
                end

                local allowed = lang_text(msg.to.id, 'allowed')
                local noAllowed = lang_text(msg.to.id, 'noAllowed')

                --Enable/disable Stickers
                local hash = 'stickers:'..msg.to.id
                if redis:get(hash) then
                    sStickers = noAllowed
                    sStickersD = '🔹'
                else
                    sStickers = allowed
                    sStickersD = '🔸'
                end
                text = text..sStickersD..' '..lang_text(msg.to.id, 'stickers')..': '..sStickers..'\n'

		--Enable/disable Tgservices
                local hash = 'tgservices:'..msg.to.id
                if redis:get(hash) then
                    tTgservices = noAllowed
                    tTgservicesD = '🔹'
                else
                    tTgservices = allowed
                    tTgservicesD = '🔸'
                end
                text = text..tTgservicesD..' '..lang_text(msg.to.id, 'tgservices')..': '..tTgservices..'\n'

                --Enable/disable Links
                local hash = 'links:'..msg.to.id
                if redis:get(hash) then
                    sLink = noAllowed
                    sLinkD = '🔹'
                else
                    sLink = allowed
                    sLinkD = '🔸'
                end
                text = text..sLinkD..' '..lang_text(msg.to.id, 'links')..': '..sLink..'\n'

                --Enable/disable arabic messages
                local hash = 'arabic:'..msg.to.id
                if not redis:get(hash) then
                    sArabe = allowed
                    sArabeD = '🔸'              
                else
                    sArabe = noAllowed
                    sArabeD = '🔹'
                end
                text = text..sArabeD..' '..lang_text(msg.to.id, 'arabic')..': '..sArabe..'\n'

                --Enable/disable bots
                local hash = 'antibot:'..msg.to.id
                if not redis:get(hash) then
                    sBots = allowed
                    sBotsD = '🔸'
                else
                    sBots = noAllowed
                    sBotsD = '🔹'
                end
                text = text..sBotsD..' '..lang_text(msg.to.id, 'bots')..': '..sBots..'\n'
                
                --Enable/disable gifs
                local hash = 'gifs:'..msg.to.id
                if redis:get(hash) then
                    sGif = noAllowed
                    sGifD = '🔹'
                else
                    sGif = allowed
                    sGifD = '🔸'
                end
                text = text..sGifD..' '..lang_text(msg.to.id, 'gifs')..': '..sGif..'\n'
                
                --Enable/disable send photos
                local hash = 'photo:'..msg.to.id
                if redis:get(hash) then
                    sPhoto = noAllowed
                    sPhotoD = '🔹'
                else
                    sPhoto = allowed
                    sPhotoD = '🔸'
                end
                text = text..sPhotoD..' '..lang_text(msg.to.id, 'photos')..': '..sPhoto..'\n'

                --Enable/disable send audios
                local hash = 'audio:'..msg.to.id
                if redis:get(hash) then
                    sAudio = noAllowed
                    sAudioD = '🔹'
                else
                    sAudio = allowed
                    sAudioD = '🔸'
                end
                text = text..sAudioD..' '..lang_text(msg.to.id, 'audios')..': '..sAudio..'\n'

                --Enable/disable kickme
                local hash = 'kickme:'..msg.to.id
                if redis:get(hash) then
                    sKickme = allowed
                    sKickmeD = '🔸'
                else
                    sKickme = noAllowed
                    sKickmeD = '🔹'
                end
                text = text..sKickmeD..' '..lang_text(msg.to.id, 'kickme')..': '..sKickme..'\n'

                --Enable/disable spam
                local hash = 'spam:'..msg.to.id
                if redis:get(hash) then
                    sSpam = noAllowed
                    sSpamD = '🔹'
                else
                    sSpam = allowed
                    sSpamD = '🔸'
                end
                text = text..sSpamD..' '..lang_text(msg.to.id, 'spam')..': '..sSpam..'\n'

                --Enable/disable setphoto
                local hash = 'setphoto:'..msg.to.id
                if not redis:get(hash) then
                    sSPhoto = allowed
                    sSPhotoD = '🔸'
                else
                    sSPhoto = noAllowed
                    sSPhotoD = '🔹'
                end
                text = text..sSPhotoD..' '..lang_text(msg.to.id, 'setphoto')..': '..sSPhoto..'\n'

                --Enable/disable changing group name
                local hash = 'name:enabled:'..msg.to.id
                if redis:get(hash) then
                    sName = noAllowed
                    sNameD = '🔹'
                else
                    sName = allowed
                    sNameD = '🔸'
                end
                text = text..sNameD..' '..lang_text(msg.to.id, 'gName')..': '..sName..'\n'

                --Lock/unlock numbers of channel members
                local hash = 'lockmember:'..msg.to.id
                if redis:get(hash) then
                    sLock = noAllowed
                    sLockD = '🔹'
                else
                    sLock = allowed
                    sLockD = '🔸'
                end
                text = text..sLockD..' lockmembers: '..sLock..'\n'

                --Enable/disable Flood
                local hash = 'anti-flood:'..msg.to.id
                if redis:get(hash) then
                    sFlood = allowed
                    sFloodD = '🔸'
                else
                    sFlood = noAllowed
                    sFloodD = '🔹'
                end
                text = text..sFloodD..' '..lang_text(msg.to.id, 'flood')..': '..sFlood..'\n'

                local hash = 'langset:'..msg.to.id
                if redis:get(hash) then
                    sLang = redis:get(hash)
                    sLangD = '🔸'
                else
                    sLang = lang_text(msg.to.id, 'noSet')
                    sLangD = '🔹'
                end
                text = text..sLangD..' '..lang_text(msg.to.id, 'language')..': '..string.upper(sLang)..'\n'

                local hash = 'flood:max:'..msg.to.id
                if not redis:get(hash) then
                    floodMax = 5
                else
                    floodMax = redis:get(hash)
                end

                local hash = 'flood:time:'..msg.to.id
                if not redis:get(hash) then
                    floodTime = 3
                else
                    floodTime = redis:get(hash)
                end

                text = text..'🔺 '..lang_text(msg.to.id, 'mFlood')..': '..floodMax..'\n🔺 '..lang_text(msg.to.id, 'tFlood')..': '..floodTime..'\n'            
                
                --Send settings to group or supergroup
                if msg.to.type == 'chat' then
                    send_msg('chat#id'..msg.to.id, text, ok_cb, false)
                elseif msg.to.type == 'channel' then
                    send_msg('channel#id'..msg.to.id, text, ok_cb, false)
                end
                return
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'rem' then
        if permissions(msg.from.id, msg.to.id, "settings") then
            if msg.reply_id then
                get_message(msg.reply_id, remove_message, false)
                get_message(msg.id, remove_message, false)
            end
            return
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'lang' then
        if permissions(msg.from.id, msg.to.id, "set_lang") then
            hash = 'langset:'..msg.to.id
            redis:set(hash, matches[2])
            return lang_text(msg.to.id, 'langUpdated')..string.upper(matches[2])
        else
            return '🚫 '..lang_text(msg.to.id, 'require_sudo')
        end
    elseif matches[1] == 'setname' then
        if permissions(msg.from.id, msg.to.id, "settings") then
            local hash = 'name:enabled:'..msg.to.id
            if not redis:get(hash) then
                if msg.to.type == 'chat' then
                    rename_chat(msg.to.peer_id, matches[2], ok_cb, false)
                elseif msg.to.type == 'channel' then
                    rename_channel(msg.to.peer_id, matches[2], ok_cb, false)
                end
            end
            return
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'setlink' then
        if permissions(msg.from.id, msg.to.id, "setlink") then
            hash = 'link:'..msg.to.id
            redis:set(hash, matches[2])
            if msg.to.type == 'chat' then
                send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'linkSaved'), ok_cb, true)
            elseif msg.to.type == 'channel' then
                send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'linkSaved'), ok_cb, true)
            end
            return
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'newlink' then
        if permissions(msg.from.id, msg.to.id, "setlink") then
        	local receiver = get_receiver(msg)
            local hash = 'link:'..msg.to.id
    		local function cb(extra, success, result)
    			if result then
    				redis:set(hash, result)
    			end
	            if success == 0 then
	                return send_large_msg(receiver, 'I can\'t create a newlink.', ok_cb, true)
	            end
    		end
    		if msg.to.type == 'chat' then
                result = export_chat_link(receiver, cb, true)
            elseif msg.to.type == 'channel' then
                result = export_channel_link(receiver, cb, true)
            end
    		if result then
	            if msg.to.type == 'chat' then
	                send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'linkSaved'), ok_cb, true)
	            elseif msg.to.type == 'channel' then
	                send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'linkSaved'), ok_cb, true)
	            end
	        end
            return
        else
            return '?? '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'link' then
        if permissions(msg.from.id, msg.to.id, "link") then
            hash = 'link:'..msg.to.id
            local linktext = redis:get(hash)
            if linktext then
                if msg.to.type == 'chat' then
                    send_msg('chat#id'..msg.to.id, '🌐 '..lang_text(msg.to.id, 'groupLink')..': '..linktext, ok_cb, true)
                elseif msg.to.type == 'channel' then
                    send_msg('channel#id'..msg.to.id, '🌐 '..lang_text(msg.to.id, 'sGroupLink')..': '..linktext, ok_cb, true)
                end
            else
                if msg.to.type == 'chat' then
                    send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noLinkSet'), ok_cb, true)
                elseif msg.to.type == 'channel' then
                    send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'noLinkSet'), ok_cb, true)
                end
            end
            return
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'setphoto' then
        if permissions(msg.from.id, msg.to.id, "settings") then
            hash = 'setphoto:'..msg.to.id
            if redis:get(hash) then
                if matches[2] == 'stop' then
                    hash = 'setphoto:'..msg.to.id..':'..msg.from.id
                    redis:del(hash)
                    if msg.to.type == 'chat' then
                        send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'setPhotoAborted'), ok_cb, true)
                    elseif msg.to.type == 'channel' then
                        send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'setPhotoAborted'), ok_cb, true)
                    end
                else
                    hash = 'setphoto:'..msg.to.id..':'..msg.from.id
                    redis:set(hash, true)
                    if msg.to.type == 'chat' then
                        send_msg('chat#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'sendPhoto'), ok_cb, true)
                    elseif msg.to.type == 'channel' then
                        send_msg('channel#id'..msg.to.id, 'ℹ️ '..lang_text(msg.to.id, 'sendPhoto'), ok_cb, true)
                    end
                end
            else
                return 'ℹ️ '..lang_text(msg.to.id, 'setPhotoError')
            end
            return
        else
            return '🚫 '..lang_text(msg.to.id, 'require_mod')
        end
    elseif matches[1] == 'tosupergroup' then
        if msg.to.type == 'chat' then
            if permissions(msg.from.id, msg.to.id, "tosupergroup") then
                chat_upgrade('chat#id'..msg.to.id, ok_cb, false)
                return 'ℹ️ '..lang_text(msg.to.id, 'chatUpgrade')
            else
                return '🚫 '..lang_text(msg.to.id, 'require_sudo')
            end
        else
            return 'ℹ️ '..lang_text(msg.to.id, 'notInChann')
        end
    elseif matches[1] == 'setdescription' then
        if permissions(msg.from.id, msg.to.id, "description") then
            local text = matches[2]
            local chat = 'channel#id'..msg.to.id
            if msg.to.type == 'channel' then
                channel_set_about(chat, text, ok_cb, false)
                return 'ℹ️ '..lang_text(msg.to.id, 'desChanged')
            else
                return 'ℹ️ '..lang_text(msg.to.id, 'desOnlyChannels')
            end
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'muteall' and matches[2] then
    	if permissions(msg.from.id, msg.to.id, "muteall") then
    		print(1)
    		local hash = 'muteall:'..msg.to.id
    		redis:setex(hash, tonumber(matches[2]), true)
    		print(2)
            return 'ℹ️ '..lang_text(msg.to.id, 'muteAllX:1')..' '..matches[2]..' '..lang_text(msg.to.id, 'muteAllX:2')
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'muteall' then
    	if permissions(msg.from.id, msg.to.id, "muteall") then
    		local hash = 'muteall:'..msg.to.id
    		redis:set(hash, true)
            return 'ℹ️ '..lang_text(msg.to.id, 'muteAll')
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'unmuteall' then
    	if permissions(msg.from.id, msg.to.id, "muteall") then
    		local hash = 'muteall:'..msg.to.id
    		redis:del(hash)
            return 'ℹ️ '..lang_text(msg.to.id, 'unmuteAll')
        else
            return '🚫 '..lang_text(msg.to.id, 'require_admin')
        end
    elseif matches[1] == 'creategroup' and matches[2] then
		if permissions(msg.from.id, msg.to.id, "creategroup") then
	            group_name = matches[2]
		    return create_group(msg, group_name)
		end
    elseif matches[1] == 'chat_created' and msg.from.id == 0 then
        return '🆕 '..lang_text(msg.to.id, 'newGroupWelcome')
    end
end

return {
    patterns = {
        '^[!/#](settings)$',
        '^[!/#](settings) (.*) (.*)$',
        '^[!/#](rem)$',
        '^[!/#](setname) (.*)$',
        '^[!/#](setphoto)$',
        '^[!/#](setphoto) (.*)$',
        '^[!/#](muteall)$',
        '^[!/#](muteall) (.*)$',
        '^[!/#](unmuteall)$',
        '^[!/#](link)$',
        '^[!/#](newlink)$',
        '^[!/#](tosupergroup)$',
        '^[!/#](setdescription) (.*)$',
        '^[!/#](setlink) (.*)$',
        '^[!/#](lang) (.*)$',
        '^[!/#](creategroup) (.*)$',
 		'^!!tgservice (.+)$'
    },
    pre_process = pre_process,
    run = run
}
end
