------------------------------------------
--  DBTeam DBTeam DBTeam DBTeam DBTeam ---
--  Welcome by @xxdamage               ---
-- multilanguage and fix by@Jarriz     ---
------------------------------------------
function chat_new_user(msg)
   local name = msg.action.user.first_name:gsub('_', ' ')
    local id = msg.action.user.id
    if msg.action.user.username then
      name = name
   end
   local chat = msg.to.print_name:gsub('_', ' ')
   local receiver = get_receiver(msg)
   local message = redis:get('welcome:'..msg.to.id)
   if not message then
      return '😀 ' ..lang_text(msg.to.id, 'welcome1') ..name.. '! ' ..lang_text(msg.to.id, 'welcome2') ..chat..'!\n🆔 ' ..id
   end
   send_msg(receiver, message, ok_cb, false)
end

local function wlc_enabled(msg)
   local var = true
   local hash = 'wlcstatus:'..msg.to.id
   local cstatus = redis:get(hash)
   if cstatus == 'off' then
      var = false
   end
   
   return var
end

local function bye_enabled(msg)
   local var = true
   local hash = 'byestatus:'..msg.to.id
   local cstatus = redis:get(hash)
   if cstatus == 'off' then
      var = false
   end
   
   return var
end

local function run(msg, matches)


	local receiver = get_receiver(msg)
	if matches[1] == "chat_add_user" then
	   if not wlc_enabled(msg) then
	      return
   end
		return chat_new_user(msg)
	elseif matches[1] == "chat_add_user_link" then
	   if not wlc_enabled(msg) then
	      return
	   end
		local name = msg.from.first_name:gsub('_', ' ')
		local chat = msg.to.print_name:gsub('_', ' ')
		local message
		if msg.from.username then
			name = name
		end
		message = redis:get('welcome:'..msg.to.id)
		if not message then
         return '😀' ..lang_text(msg.to.id, 'welcome1') ..name.. '!' ..lang_text(msg.to.id, 'welcome2') ..chat..'!\n🆔 ' ..id
      end
		send_msg(receiver, message, ok_cb, false)
	elseif matches[1] == "chat_del_user" then
	   if not bye_enabled(msg) then
	      return
	   end
		local name = msg.action.user.first_name:gsub('_', ' ')
		if msg.action.user.username then
			name = name
		end
		local message = redis:get('bye:'..msg.to.id)
		if not message then
         return '😀 ' ..lang_text(msg.to.id, 'bye1') ..name.. '!' ..lang_text(msg.to.id, 'bye2')
      end
		send_msg(receiver, message, ok_cb, false)
   elseif matches[1] == 'setwelcome' then
		if not permissions(msg.from.id, msg.to.id, "welcome") then
			return '🚫 '..lang_text(msg.to.id, 'require_mod')
		end
      print(msg.to.id)
      local hash = 'welcome:'..msg.to.id
      redis:set(hash, matches[2])
      return '✅ ' ..lang_text(msg.to.id, 'welnew') .. ': \n'  ..matches[2]
   elseif matches[1] == 'getwelcome' then
      print(msg.to.id)
      local hash = 'welcome:'..msg.to.id
      local wel = redis:get(hash)
      if not wel then
         return 'ℹ️ ' ..lang_text(msg.to.id, 'weldefault')
      end
      return wel
   elseif matches[1] == 'setbye' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return ' 🚫'..lang_text(msg.to.id, 'require_mod')
		end
      print(msg.to.id)
      local hash = 'bye:'..msg.to.id
      redis:set(hash, matches[2])
      return '✅ ' ..lang_text(msg.to.id, 'newbye') .. ':\n'..matches[2]
   elseif matches[1] == 'getbye' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return ' 🚫'..lang_text(msg.to.id, 'require_mod')
		end
      print(msg.to.id)
      local hash = 'bye:'..msg.to.id
      local wel = redis:get(hash)
      if not wel then
         return 'ℹ️ ' ..lang_text(msg.to.id, 'byedefault')
      end
      return wel
   elseif matches[1] == 'welcome on' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return ' 🚫'..lang_text(msg.to.id, 'require_mod')
		end
      local hash = 'wlcstatus:'..msg.to.id
      redis:set(hash, 'on')
      return 'ℹ️ '..lang_text(msg.to.id, 'welon')
   elseif matches[1] == 'welcome off' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return '🚫 '..lang_text(msg.to.id, 'require_mod')
		end
      local hash = 'wlcstatus:'..msg.to.id
      redis:set(hash, 'off')
      return 'ℹ️ '..lang_text(msg.to.id, 'weloff')
   elseif matches[1] == 'bye on' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return ' 🚫'..lang_text(msg.to.id, 'require_mod')
		end
      local hash = 'byestatus:'..msg.to.id
      redis:set(hash, 'on')
      return 'ℹ️ '..lang_text(msg.to.id, 'byeon')
   elseif matches[1] == 'bye off' then
   if not permissions(msg.from.id, msg.to.id, "welcome") then
			return ' 🚫'..lang_text(msg.to.id, 'require_mod')
		end
      local hash = 'byestatus:'..msg.to.id
      redis:set(hash, 'off')
      return 'ℹ️ '..lang_text(msg.to.id, 'byeoff')
   end
end

return {
   description = "Service plugin that sends a custom message when an user enters a chat.",
   usage = "",
   patterns = {
	"^!!tgservice (chat_add_user)$",
	"^!!tgservice (chat_del_user)$",
	"^!!tgservice (chat_add_user_link)$",
	"^[!/#](setwelcome) (.*)",
	"^[!/#](getwelcome)",
	"^[!/#](setbye) (.*)",
	"^[!/#](getbye)",
	"^[!/#](welcome on)",
	"^[!/#](welcome off)",
	"^[!/#](bye on)",
	"^[!/#](bye off)"
   },
   run = run
}
