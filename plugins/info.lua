local function callback_reply(extra, success, result)
	--icon & rank ------------------------------------------------------------------------------------------------
	userrank = "Member"
	if tonumber(result.from.id) == 139693972 then
		userrank = "Master ⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/7.webp", ok_cb, false)
	elseif is_sudo(result) then
		userrank = "Sudo ⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/8.webp", ok_cb, false)
	elseif is_admin(result) then
		userrank = "Admin ⭐⭐⭐"
		send_document(org_chat_id,"./icons/3.webp", ok_cb, false)
	elseif tonumber(result.from.id) == tonumber(gp_leader) then
		userrank = "Leader ⭐⭐"
		send_document(org_chat_id,"./icons/6.webp", ok_cb, false)
	elseif is_momod(result) then
		userrank = "Moderator ⭐"
		send_document(org_chat_id,"./icons/4.webp", ok_cb, false)
	elseif tonumber(result.from.id) == tonumber(our_id) then
		userrank = "Umbrella ⭐⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/9.webp", ok_cb, false)
	elseif result.from.username then
		if string.sub(result.from.username:lower(), -3) == "bot" then
			userrank = "API Bot"
			send_document(org_chat_id,"./icons/5.webp", ok_cb, false)
		end
	end
	--custom rank ------------------------------------------------------------------------------------------------
	local file = io.open("./info/"..result.from.id..".txt", "r")
	if file ~= nil then
		usertype = file:read("*all")
	else
		usertype = "-----"
	end
	--cont ------------------------------------------------------------------------------------------------
	local user_info = {}
	local uhash = 'user:'..result.from.id
	local user = redis:hgetall(uhash)
	local um_hash = 'msgs:'..result.from.id..':'..result.to.id
	user_info.msgs = tonumber(redis:get(um_hash) or 0)
	--msg type ------------------------------------------------------------------------------------------------
	if result.media then
		if result.media.type == "document" then
			if result.media.text then
				msg_type = "استیکر"
			else
				msg_type = "ساير فايلها"
			end
		elseif result.media.type == "photo" then
			msg_type = "فايل عکس"
		elseif result.media.type == "video" then
			msg_type = "فايل ويدئويي"
		elseif result.media.type == "audio" then
			msg_type = "فايل صوتي"
		elseif result.media.type == "geo" then
			msg_type = "موقعيت مکاني"
		elseif result.media.type == "contact" then
			msg_type = "شماره تلفن"
		elseif result.media.type == "file" then
			msg_type = "فايل"
		elseif result.media.type == "webpage" then
			msg_type = "پیش نمایش سایت"
		elseif result.media.type == "unsupported" then
			msg_type = "فايل متحرک"
		else
			msg_type = "ناشناخته"
		end
	elseif result.text then
		if string.match(result.text, '^%d+$') then
			msg_type = "عدد"
		elseif string.match(result.text, '%d+') then
			msg_type = "شامل عدد و حروف"
		elseif string.match(result.text, '^@') then
			msg_type = "یوزرنیم"
		elseif string.match(result.text, '@') then
			msg_type = "شامل یوزرنیم"
		elseif string.match(result.text, '[Tt][Ee][Ll][Ee][Gg][Rr][Aa][Mm].[Mm][Ee]') then
			msg_type = "لينک تلگرام"
		elseif string.match(result.text, '[Hh][Tt][Tt][Pp]') then
			msg_type = "لينک سايت"
		elseif string.match(result.text, '[Ww][Ww][Ww]') then
			msg_type = "لينک سايت"
		elseif string.match(result.text, '?') then
			msg_type = "پرسش"
		else
			msg_type = "متن"
		end
	end
	--hardware ------------------------------------------------------------------------------------------------
	if result.text then
		inputtext = string.sub(result.text, 0,1)
		if result.text then
			if string.match(inputtext, "[a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z]") then
				hardware = "کامپیوتر"
			elseif string.match(inputtext, "[A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z]") then
				hardware = "موبایل"
			else
				hardware = "-----"
			end
		else
			hardware = "-----"
		end
	else
		hardware = "-----"
	end
	--phone ------------------------------------------------------------------------------------------------
	if access == 1 then
		if result.from.phone then
			number = "0"..string.sub(result.from.phone, 3)
			if string.sub(result.from.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.from.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.from.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.from.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.from.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.from.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	elseif access == 0 then
		if result.from.phone then
			number = "شما مجاز نیستید"
			if string.sub(result.from.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.from.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.from.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.from.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.from.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.from.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	end
	--info ------------------------------------------------------------------------------------------------
	info = "نام کامل: "..string.gsub(result.from.print_name, "_", " ").."\n"
	.."نام کوچک: "..(result.from.first_name or "-----").."\n"
	.."نام خانوادگی: "..(result.from.last_name or "-----").."\n\n"
	.."شماره موبایل: "..number.."\n"
	.."یوزرنیم: @"..(result.from.username or "-----").."\n"
	.."آی دی: "..result.from.id.."\n\n"
	.."مقام: "..usertype.."\n"
	.."جایگاه: "..userrank.."\n\n"
	.."رابط کاربری: "..hardware.."\n"
	.."تعداد پیامها: "..user_info.msgs.."\n"
	.."نوع پیام: "..msg_type.."\n\n"
	.."نام گروه: "..string.gsub(result.to.print_name, "_", " ").."\n"
	.."آی دی گروه: "..result.to.id
	send_large_msg(org_chat_id, info)
end

local function callback_res(extra, success, result)
	if success == 0 then
		return send_large_msg(org_chat_id, "یوزرنیم وارد شده اشتباه است")
	end
	--icon & rank ------------------------------------------------------------------------------------------------
	if tonumber(result.id) == 139693972 then
		userrank = "Master ⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/7.webp", ok_cb, false)
	elseif is_sudo(result) then
		userrank = "Sudo ⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/8.webp", ok_cb, false)
	elseif is_admin(result) then
		userrank = "Admin ⭐⭐⭐"
		send_document(org_chat_id,"./icons/3.webp", ok_cb, false)
	elseif tonumber(result.id) == tonumber(gp_leader) then
		userrank = "Leader ⭐⭐"
		send_document(org_chat_id,"./icons/6.webp", ok_cb, false)
	elseif is_momod(result) then
		userrank = "Moderator ⭐"
		send_document(org_chat_id,"./icons/4.webp", ok_cb, false)
	elseif tonumber(result.id) == tonumber(our_id) then
		userrank = "Umbrella ⭐⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/9.webp", ok_cb, false)
	elseif string.sub(result.username:lower(), -3) == 'bot' then
		userrank = "API Bot"
		send_document(org_chat_id,"./icons/5.webp", ok_cb, false)
	else
		userrank = "Member"
	end
	--custom rank ------------------------------------------------------------------------------------------------
	local file = io.open("./info/"..result.id..".txt", "r")
	if file ~= nil then
		usertype = file:read("*all")
	else
		usertype = "-----"
	end
	--phone ------------------------------------------------------------------------------------------------
	if access == 1 then
		if result.phone then
			number = "0"..string.sub(result.phone, 3)
			if string.sub(result.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	elseif access == 0 then
		if result.phone then
			number = "شما مجاز نیستید"
			if string.sub(result.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	end
	--info ------------------------------------------------------------------------------------------------
	info = "نام کامل: "..string.gsub(result.print_name, "_", " ").."\n"
	.."نام کوچک: "..(result.first_name or "-----").."\n"
	.."نام خانوادگی: "..(result.last_name or "-----").."\n\n"
	.."شماره موبایل: "..number.."\n"
	.."یوزرنیم: @"..(result.username or "-----").."\n"
	.."آی دی: "..result.id.."\n\n"
	.."مقام: "..usertype.."\n"
	.."جایگاه: "..userrank.."\n\n"
	send_large_msg(org_chat_id, info)
end

local function callback_info(extra, success, result)
	if success == 0 then
		return send_large_msg(org_chat_id, "آی دی وارد شده اشتباه است")
	end
	--icon & rank ------------------------------------------------------------------------------------------------
	if tonumber(result.id) == 139693972 then
		userrank = "Master ⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/7.webp", ok_cb, false)
	elseif is_sudo(result) then
		userrank = "Sudo ⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/8.webp", ok_cb, false)
	elseif is_admin(result) then
		userrank = "Admin ⭐⭐⭐"
		send_document(org_chat_id,"./icons/3.webp", ok_cb, false)
	elseif tonumber(result.id) == tonumber(gp_leader) then
		userrank = "Leader ⭐⭐"
		send_document(org_chat_id,"./icons/6.webp", ok_cb, false)
	elseif is_momod(result) then
		userrank = "Moderator ⭐"
		send_document(org_chat_id,"./icons/4.webp", ok_cb, false)
	elseif tonumber(result.id) == tonumber(our_id) then
		userrank = "Umbrella ⭐⭐⭐⭐⭐⭐"
		send_document(org_chat_id,"./icons/9.webp", ok_cb, false)
	elseif string.sub(result.username:lower(), -3) == 'bot' then
		userrank = "API Bot"
		send_document(org_chat_id,"./icons/5.webp", ok_cb, false)
	else
		userrank = "Member"
	end
	--custom rank ------------------------------------------------------------------------------------------------
	local file = io.open("./info/"..result.id..".txt", "r")
	if file ~= nil then
		usertype = file:read("*all")
	else
		usertype = "-----"
	end
	--phone ------------------------------------------------------------------------------------------------
	if access == 1 then
		if result.phone then
			number = "0"..string.sub(result.phone, 3)
			if string.sub(result.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	elseif access == 0 then
		if result.phone then
			number = "شما مجاز نیستید"
			if string.sub(result.phone, 0,2) == '98' then
				number = number.."\nکشور: جمهوری اسلامی ایران"
				if string.sub(result.phone, 0,4) == '9891' then
					number = number.."\nنوع سیمکارت: همراه اول"
				elseif string.sub(result.phone, 0,5) == '98932' then
					number = number.."\nنوع سیمکارت: تالیا"
				elseif string.sub(result.phone, 0,4) == '9893' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9890' then
					number = number.."\nنوع سیمکارت: ایرانسل"
				elseif string.sub(result.phone, 0,4) == '9892' then
					number = number.."\nنوع سیمکارت: رایتل"
				else
					number = number.."\nنوع سیمکارت: سایر"
				end
			else
				number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
			end
		else
			number = "-----"
		end
	end
	--name ------------------------------------------------------------------------------------------------
	if string.len(result.print_name) > 15 then
		fullname = string.sub(result.print_name, 0,15).."..."
	else
		fullname = result.print_name
	end
	if result.first_name then
		if string.len(result.first_name) > 15 then
			firstname = string.sub(result.first_name, 0,15).."..."
		else
			firstname = result.first_name
		end
	else
		firstname = "-----"
	end
	if result.last_name then
		if string.len(result.last_name) > 15 then
			lastname = string.sub(result.last_name, 0,15).."..."
		else
			lastname = result.last_name
		end
	else
		lastname = "-----"
	end
	--info ------------------------------------------------------------------------------------------------
	info = "نام کامل: "..string.gsub(result.print_name, "_", " ").."\n"
	.."نام کوچک: "..(result.first_name or "-----").."\n"
	.."نام خانوادگی: "..(result.last_name or "-----").."\n\n"
	.."شماره موبایل: "..number.."\n"
	.."یوزرنیم: @"..(result.username or "-----").."\n"
	.."آی دی: "..result.id.."\n\n"
	.."مقام: "..usertype.."\n"
	.."جایگاه: "..userrank.."\n\n"
	send_large_msg(org_chat_id, info)
end

local function run(msg, matches)
	local data = load_data(_config.moderation.data)
	gp_leader = data[tostring(msg.to.id)]['settings']['gp_leader']
	org_chat_id = "chat#id"..msg.to.id
	if is_sudo(msg) then
		access = 1
	else
		access = 0
	end
	if matches[1] == '/infodel' and is_sudo(msg) then
		azlemagham = io.popen('rm ./info/'..matches[2]..'.txt'):read('*all')
		return 'از مقام خود عزل شد'
	elseif matches[1] == '/info' and is_sudo(msg) then
		local name = string.sub(matches[2], 1, 50)
		local text = string.sub(matches[3], 1, 10000000000)
		local file = io.open("./info/"..name..".txt", "w")
		file:write(text)
		file:flush()
		file:close() 
		return "مقام ثبت شد"
	elseif #matches == 2 then
		local cbres_extra = {chatid = msg.to.id}
		if string.match(matches[2], '^%d+$') then
			return user_info('user#id'..matches[2], callback_info, cbres_extra)
		else
			return res_user(matches[2]:gsub("@",""), callback_res, cbres_extra)
		end
	else
		--custom rank ------------------------------------------------------------------------------------------------
		local file = io.open("./info/"..msg.from.id..".txt", "r")
		if file ~= nil then
			usertype = file:read("*all")
		else
			usertype = "-----"
		end
		--hardware ------------------------------------------------------------------------------------------------
		if matches[1] == "info" then
			hardware = "کامپیوتر"
		else
			hardware = "موبایل"
		end
		if not msg.reply_id then
			--contor ------------------------------------------------------------------------------------------------
			local user_info = {}
			local uhash = 'user:'..msg.from.id
			local user = redis:hgetall(uhash)
			local um_hash = 'msgs:'..msg.from.id..':'..msg.to.id
			user_info.msgs = tonumber(redis:get(um_hash) or 0)
			--icon & rank ------------------------------------------------------------------------------------------------
			if tonumber(msg.from.id) == 139693972 then
				userrank = "Master ⭐⭐⭐⭐"
				send_document("chat#id"..msg.to.id,"./icons/7.webp", ok_cb, false)
			elseif is_sudo(msg) then
				userrank = "Sudo ⭐⭐⭐⭐⭐"
				send_document("chat#id"..msg.to.id,"./icons/8.webp", ok_cb, false)
			elseif is_admin(msg) then
				userrank = "Admin ⭐⭐⭐"
				send_document("chat#id"..msg.to.id,"./icons/3.webp", ok_cb, false)
			elseif tonumber(msg.from.id) == tonumber(gp_leader) then
				userrank = "Leader ⭐⭐"
				send_document("chat#id"..msg.to.id,"./icons/6.webp", ok_cb, false)
			elseif is_momod(msg) then
				userrank = "Moderator ⭐"
				send_document("chat#id"..msg.to.id,"./icons/4.webp", ok_cb, false)
			else
				userrank = "Member"
			end
			--number ------------------------------------------------------------------------------------------------
			if msg.from.phone then
				numberorg = string.sub(msg.from.phone, 3)
				number = "****0"..string.sub(numberorg, 0,6)
				if string.sub(msg.from.phone, 0,2) == '98' then
					number = number.."\nکشور: جمهوری اسلامی ایران"
					if string.sub(msg.from.phone, 0,4) == '9891' then
						number = number.."\nنوع سیمکارت: همراه اول"
					elseif string.sub(msg.from.phone, 0,5) == '98932' then
						number = number.."\nنوع سیمکارت: تالیا"
					elseif string.sub(msg.from.phone, 0,4) == '9893' then
						number = number.."\nنوع سیمکارت: ایرانسل"
					elseif string.sub(msg.from.phone, 0,4) == '9890' then
						number = number.."\nنوع سیمکارت: ایرانسل"
					elseif string.sub(msg.from.phone, 0,4) == '9892' then
						number = number.."\nنوع سیمکارت: رایتل"
					else
						number = number.."\nنوع سیمکارت: سایر"
					end
				else
					number = number.."\nکشور: خارج\nنوع سیمکارت: متفرقه"
				end
			else
				number = "-----"
			end
			--info ------------------------------------------------------------------------------------------------
			local info = "نام کامل: "..string.gsub(msg.from.print_name, "_", " ").."\n"
					.."نام کوچک: "..(msg.from.first_name or "-----").."\n"
					.."نام خانوادگی: "..(msg.from.last_name or "-----").."\n\n"
					.."شماره موبایل: "..number.."\n"
					.."یوزرنیم: @"..(msg.from.username or "-----").."\n"
					.."آی دی: "..msg.from.id.."\n\n"
					.."مقام: "..usertype.."\n"
					.."جایگاه: "..userrank.."\n\n"
					.."رابط کاربری: "..hardware.."\n"
					.."تعداد پیامها: "..user_info.msgs.."\n\n"
					.."نام گروه: "..string.gsub(msg.to.print_name, "_", " ").."\n"
					.."آی دی گروه: "..msg.to.id
			return info
		else
			get_message(msg.reply_id, callback_reply, false)
		end
	end
end

return {
	description = "User Infomation",
	usagehtm = '<tr><td align="center">info</td><td align="right">اطلاعات کاملی را راجبه شما، گروهی که در آن هستید و مقامتان میدهد همچنین با رپلی کردن میتوانید اطلاعات فرد مورد نظر را نیز ببینید</td></tr>'
	..'<tr><td align="center">/info مقام آیدی</td><td align="right">اعطای مقام به شخص به جر مقامهای اصلی</td></tr>'
	..'<tr><td align="center">/infodel آیدی</td><td align="right">حذف مقام اعطا شده</td></tr>',
	usage = {
		user = {
			"info: اطلاعات شما",
			"info (reply): اطلاعات دیگران",
			},
		sudo = {
			"/info (id) (txt) : اعطای مقام",
			"/infodel : حذف مقام",
			},
		},
	patterns = {
		"^(/infodel) (.*)$",
		"^(/info) ([^%s]+) (.*)$",
		"^([Ii]nfo) (.*)$",
		"^(info)$",
		"^(Info)$",
	},
	run = run,
}