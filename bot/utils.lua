URL = require "socket.url"
http = require "socket.http"
https = require "ssl.https"
ltn12 = require "ltn12"
serpent = require "serpent"
feedparser = require "feedparser"

json = (loadfile "./libs/JSON.lua")()
mimetype = (loadfile "./libs/mimetype.lua")()
redis = (loadfile "./libs/redis.lua")()
JSON = (loadfile "./libs/dkjson.lua")()

http.TIMEOUT = 10

function get_receiver(msg)
  if msg.to.type == 'user' then
    return 'user#id'..msg.from.id
  end
  if msg.to.type == 'chat' then
    return 'chat#id'..msg.to.id
  end
  if msg.to.type == 'encr_chat' then
    return msg.to.print_name
  end
end

function is_chat_msg( msg )
  if msg.to.type == 'chat' then
    return true
  end
  return false
end

function string.random(length)
   local str = "";
   for i = 1, length do
      math.random(97, 122)
      str = str..string.char(math.random(97, 122));
   end
   return str;
end

function string:split(sep)
  local sep, fields = sep or ":", {}
  local pattern = string.format("([^%s]+)", sep)
  self:gsub(pattern, function(c) fields[#fields+1] = c end)
  return fields
end

-- DEPRECATED
function string.trim(s)
  print("string.trim(s) is DEPRECATED use string:trim() instead")
  return s:gsub("^%s*(.-)%s*$", "%1")
end

-- Removes spaces



function vardump(value)
  print(serpent.block(value, {comment=false}))
end



-- http://www.lua.org/manual/5.2/manual.html#pdf-io.popen
function run_command(str)
  local cmd = io.popen(str)
  local result = cmd:read('*all')
  cmd:close()
  return result
end

-- User has privileges
function is_sudo(msg)
  local var = false
  -- Check users id in config
  for v,user in pairs(_config.sudo_users) do
    if user == msg.from.id then
      var = true
    end
  end
  return var
end
--Check if this chat is realm or not
function is_realm(msg)
  local var = false
  for v,group in pairs(_config.realm) do
    if group == msg.to.id then
      var = true
    end
  end
  return var
end


function savelog(group, logtxt)

local text = (os.date("[ %c ]=>  "..logtxt.."\n \n"))
local file = io.open("./groups/"..group.."log.txt", "a")

file:write(text)

file:close()

end

function user_print_name(user)
   if user.print_name then
      return user.print_name
   end
   local text = ''
   if user.first_name then
      text = user.last_name..' '
   end
   if user.lastname then
      text = text..user.last_name
   end
   return text
end

--Check if user is the owner of that group or not
function is_owner(msg)
  local var = false
  local data = load_data(_config.moderation.data)
  local user = msg.from.id
  
  if data[tostring(msg.to.id)] then
    if data[tostring(msg.to.id)]['set_owner'] then
      if data[tostring(msg.to.id)]['set_owner'] == tostring(user) then
        var = true
      end
    end
  end

  if data['admins'] then
    if data['admins'][tostring(user)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == msg.from.id then
        var = true
    end
  end
  return var
end

function is_owner2(user_id, group_id)
  local var = false
  local data = load_data(_config.moderation.data)

  if data[tostring(group_id)] then
    if data[tostring(group_id)]['set_owner'] then
      if data[tostring(group_id)]['set_owner'] == tostring(user_id) then
        var = true
      end
    end
  end
  
  if data['admins'] then
    if data['admins'][tostring(user_id)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == user_id then
        var = true
    end
  end
  return var
end

--Check if user is admin or not
function is_admin(msg)
  local var = false
  local data = load_data(_config.moderation.data)
  local user = msg.from.id
  local admins = 'admins'
  if data[tostring(admins)] then
    if data[tostring(admins)][tostring(user)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == msg.from.id then
        var = true
    end
  end
  return var
end

function is_admin2(user_id)
  local var = false
  local data = load_data(_config.moderation.data)
  local user = user_id
  local admins = 'admins'
  if data[tostring(admins)] then
    if data[tostring(admins)][tostring(user)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == user_id then
        var = true
    end
  end
  return var
end



--Check if user is the mod of that group or not
function is_momod(msg)
  local var = false
  local data = load_data(_config.moderation.data)
  local user = msg.from.id
  
  if data[tostring(msg.to.id)] then
    if data[tostring(msg.to.id)]['moderators'] then
      if data[tostring(msg.to.id)]['moderators'][tostring(user)] then
        var = true
      end
    end
  end

  if data[tostring(msg.to.id)] then
    if data[tostring(msg.to.id)]['set_owner'] then
      if data[tostring(msg.to.id)]['set_owner'] == tostring(user) then
        var = true
      end
    end
  end

  if data['admins'] then
    if data['admins'][tostring(user)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == msg.from.id then
        var = true
    end
  end
  return var
end

function is_momod2(user_id, group_id)
  local var = false
  local data = load_data(_config.moderation.data)
  local usert = user_id
  if data[tostring(group_id)] then
    if data[tostring(group_id)]['moderators'] then
      if data[tostring(group_id)]['moderators'][tostring(usert)] then
        var = true
      end
    end
  end

  if data[tostring(group_id)] then
    if data[tostring(group_id)]['set_owner'] then
      if data[tostring(group_id)]['set_owner'] == tostring(user_id) then
        var = true
      end
    end
  end
  
  if data['admins'] then
    if data['admins'][tostring(user_id)] then
      var = true
    end
  end
  for v,user in pairs(_config.sudo_users) do
    if user == usert then
        var = true
    end
  end
  return var
end

-- Returns the name of the sender
function get_name(msg)
  local name = msg.from.first_name
  if name == nil then
    name = msg.from.id
  end
  return name
end

-- Returns at table of lua files inside plugins
function plugins_names( )
  local files = {}
  for k, v in pairs(scandir("plugins")) do
    -- Ends with .lua
    if (v:match(".lua$")) then
      table.insert(files, v)
    end
  end
  return files
end

-- Function name explains what it does.
function file_exists(name)
  local f = io.open(name,"r")
  if f ~= nil then
    io.close(f)
    return true
  else
    return false
  end
end

-- Save into file the data serialized for lua.
-- Set uglify true to minify the file.
function serialize_to_file(data, file, uglify)
  file = io.open(file, 'w+')
  local serialized
  if not uglify then
    serialized = serpent.block(data, {
        comment = false,
        name = '_'
      })
  else
    serialized = serpent.dump(data)
  end
  file:write(serialized)
  file:close()
end

-- Returns true if the string is empty
function string:isempty()
  return self == nil or self == ''
end

-- Returns true if the string is blank
function string:isblank()
  self = self:trim()
  return self:isempty()
end

-- DEPRECATED!!!!!
function string.starts(String, Start)
  print("string.starts(String, Start) is DEPRECATED use string:starts(text) instead")
  return Start == string.sub(String,1,string.len(Start))
end

-- Returns true if String starts with Start
function string:starts(text)
  return text == string.sub(self,1,string.len(text))
end

-- Callback to remove a file
function rmtmp_cb(cb_extra, success, result)
  local file_path = cb_extra.file_path
  local cb_function = cb_extra.cb_function or ok_cb
  local cb_extra = cb_extra.cb_extra

  if file_path ~= nil then
    os.remove(file_path)
    print("Deleted: "..file_path)
  end
  -- Finally call the callback
  cb_function(cb_extra, success, result)
end

-- Send document to user and delete it when finished.
-- cb_function and cb_extra are optionals callback
function _send_document(receiver, file_path, cb_function, cb_extra)
  local cb_extra = {
    file_path = file_path,
    cb_function = cb_function or ok_cb,
    cb_extra = cb_extra or false
  }
  -- Call to remove with optional callback
  send_document(receiver, file_path, rmtmp_cb, cb_extra)
end

-- Download the image and send to receiver, it will be deleted.
-- cb_function and cb_extra are optionals callback
function send_document_from_url(receiver, url, cb_function, cb_extra)
  local file_path = download_to_file(url, false)
  print("File path: "..file_path)
  _send_document(receiver, file_path, cb_function, cb_extra)
end

-- Parameters in ?a=1&b=2 style
function format_http_params(params, is_get)
  local str = ''
  -- If is get add ? to the beginning
  if is_get then str = '?' end
  local first = true -- Frist param
  for k,v in pairs (params) do
    if v then -- nil value
      if first then
        first = false
        str = str..k.. "="..v
      else
        str = str.."&"..k.. "="..v
      end
    end
  end
  return str
end

-- Check if user can use the plugin and warns user
-- Returns true if user was warned and false if not warned (is allowed)
function warns_user_not_allowed(plugin, msg)
  if not user_allowed(plugin, msg) then
    return true
  else
    return false
  end
end

-- Check if user can use the plugin
function user_allowed(plugin, msg)
  if plugin.privileged and not is_sudo(msg) then
    return false
  end
  return true
end


function send_order_msg(destination, msgs)
   local cb_extra = {
      destination = destination,
      msgs = msgs
   }
   send_order_msg_callback(cb_extra, true)
end

function send_order_msg_callback(cb_extra, success, result)
   local destination = cb_extra.destination
   local msgs = cb_extra.msgs
   local file_path = cb_extra.file_path
   if file_path ~= nil then
      os.remove(file_path)
      print("Deleted: " .. file_path)
   end
   if type(msgs) == 'string' then
      send_large_msg(destination, msgs)
   elseif type(msgs) ~= 'table' then
      return
   end
   if #msgs < 1 then
      return
   end
   local msg = table.remove(msgs, 1)
   local new_cb_extra = {
      destination = destination,
      msgs = msgs
   }
   if type(msg) == 'string' then
      send_msg(destination, msg, send_order_msg_callback, new_cb_extra)
   elseif type(msg) == 'table' then
      local typ = msg[1]
      local nmsg = msg[2]
      new_cb_extra.file_path = nmsg
      if typ == 'document' then
         send_document(destination, nmsg, send_order_msg_callback, new_cb_extra)
      elseif typ == 'image' or typ == 'photo' then
         send_photo(destination, nmsg, send_order_msg_callback, new_cb_extra)
      elseif typ == 'audio' then
         send_audio(destination, nmsg, send_order_msg_callback, new_cb_extra)
      elseif typ == 'video' then
         send_video(destination, nmsg, send_order_msg_callback, new_cb_extra)
      else
         send_file(destination, nmsg, send_order_msg_callback, new_cb_extra)
      end
   end
end

-- Same as send_large_msg_callback but friendly params
function send_large_msg(destination, text)
  local cb_extra = {
    destination = destination,
    text = text
  }
  send_large_msg_callback(cb_extra, true)
end

-- If text is longer than 4096 chars, send multiple msg.
-- https://core.telegram.org/method/messages.sendMessage
function send_large_msg_callback(cb_extra, success, result)
  local text_max = 4096

  local destination = cb_extra.destination
  local text = cb_extra.text
  local text_len = string.len(text)
  local num_msg = math.ceil(text_len / text_max)

  if num_msg <= 1 then
    send_msg(destination, text, ok_cb, false)
  else

    local my_text = string.sub(text, 1, 4096)
    local rest = string.sub(text, 4096, text_len)

    local cb_extra = {
      destination = destination,
      text = rest
    }

    send_msg(destination, my_text, send_large_msg_callback, cb_extra)
  end
end

-- Returns a table with matches or nil
function match_pattern(pattern, text, lower_case)
  if text then
    local matches = {}
    if lower_case then
      matches = { string.match(text:lower(), pattern) }
    else
      matches = { string.match(text, pattern) }
    end
      if next(matches) then
        return matches
      end
  end
  -- nil
end

-- Function to read data from files
function load_from_file(file, default_data)
  local f = io.open(file, "r+")
  -- If file doesn't exists
  if f == nil then
    -- Create a new empty table
    default_data = default_data or {}
    serialize_to_file(default_data, file)
    print ('Created file', file)
  else
    print ('Data loaded from file', file)
    f:close()
  end
  return loadfile (file)()
end

-- Kick
function kick_user(user_id, chat_id) 
  if tonumber(user_id) == tonumber(our_id) then -- Ignore bot
    return
  end
  if is_owner2(user_id, chat_id) then -- Ignore admins
    return
  end
  local chat = 'chat#id'..chat_id
  local user = 'user#id'..user_id
  chat_del_user(chat, user, ok_cb, true)
end

-- Ban
function ban_user(user_id, chat_id)
  if tonumber(user_id) == tonumber(our_id) then -- Ignore bot
    return
  end
  if is_admin2(user_id) then -- Ignore admins
    return
  end
  -- Save to redis
  local hash =  'banned:'..chat_id
  redis:sadd(hash, user_id)
  -- Kick from chat
  kick_user(user_id, chat_id)
end
-- Global ban
function banall_user(user_id)  
  if tonumber(user_id) == tonumber(our_id) then -- Ignore bot
    return
  end
  if is_admin2(user_id) then -- Ignore admins
    return
  end
  -- Save to redis
  local hash =  'gbanned'
  redis:sadd(hash, user_id)
end
-- Global unban
function unbanall_user(user_id)
  --Save on redis  
  local hash =  'gbanned'
  redis:srem(hash, user_id)
end

-- Check if user_id is banned in chat_id or not
function is_banned(user_id, chat_id)
  --Save on redis  
  local hash =  'banned:'..chat_id
  local banned = redis:sismember(hash, user_id)
  return banned or false
end

-- Check if user_id is globally banned or not
function is_gbanned(user_id)
  --Save on redis
  local hash =  'gbanned'
  local banned = redis:sismember(hash, user_id)
  return banned or false
end

-- Returns chat_id ban list
function ban_list(chat_id)
  local hash =  'banned:'..chat_id
  local list = redis:smembers(hash)
  local text = "Ban list !\n\n"
  for k,v in pairs(list) do
    text = text..k.." - "..v.." \n"
  end
  return text
end

-- Returns globally ban list
function banall_list() 
  local hash =  'gbanned'
  local list = redis:smembers(hash)
  local text = "global bans !\n\n"
  for k,v in pairs(list) do
    text = text..k.." - "..v.." \n"
  end
  return text
end

-- /id by reply
function get_id_by_reply(extra, success, result)
  if result.to.type == 'chat' then
    send_large_msg(chat, result.from.id)
  else
    return 'Use This in Your Groups'
  end
end

-- kick by reply for mods and owner
function Kick_by_reply(extra, success, result)
  if result.to.type == 'chat' then
    local chat = 'chat#id'..result.to.id
    if tonumber(result.from.id) == tonumber(our_id) then -- Ignore bot
      return "I won't kick myself"
    end
    if is_momod2(result.from.id, result.to.id) then -- Ignore mods,owner,admin
      return "you can't kick mods,owner and admins"
    end
    chat_del_user(chat, 'user#id'..result.from.id, ok_cb, false)
  else
    return 'Use This in Your Groups'
  end
end

-- Kick by reply for admins
function Kick_by_reply_admins(extra, success, result)
  if result.to.type == 'chat' then
    local chat = 'chat#id'..result.to.id
    if tonumber(result.from.id) == tonumber(our_id) then -- Ignore bot
      return "I won't kick myself"
    end
    if is_admin2(result.from.id) then -- Ignore admins
      return
    end
    chat_del_user(chat, 'user#id'..result.from.id, ok_cb, false)
  else
    return 'Use This in Your Groups'
  end
end

--Ban by reply for admins
function ban_by_reply(extra, success, result)
  if result.to.type == 'chat' then
  local chat = 'chat#id'..result.to.id
  if tonumber(result.from.id) == tonumber(our_id) then -- Ignore bot
      return "I won't ban myself"
  end
  if is_momod2(result.from.id, result.to.id) then -- Ignore mods,owner,admin
    return "you can't kick mods,owner and admins"
  end
  ban_user(result.from.id, result.to.id)
  send_large_msg(chat, "User "..result.from.id.." Banned")
  else
    return 'Use This in Your Groups'
  end
end

-- Ban by reply for admins
function ban_by_reply_admins(extra, success, result)
  if result.to.type == 'chat' then
    local chat = 'chat#id'..result.to.id
    if tonumber(result.from.id) == tonumber(our_id) then -- Ignore bot
      return "I won't ban myself"
    end
    if is_admin2(result.from.id) then -- Ignore admins
      return
    end
    ban_user(result.from.id, result.to.id)
    send_large_msg(chat, "User "..result.from.id.." Banned")
  else
    return 'Use This in Your Groups'
  end
end

-- Unban by reply
function unban_by_reply(extra, success, result) 
  if result.to.type == 'chat' then
    local chat = 'chat#id'..result.to.id
    if tonumber(result.from.id) == tonumber(our_id) then -- Ignore bot
      return "I won't unban myself"
    end
    send_large_msg(chat, "User "..result.from.id.." Unbanned")
    -- Save on redis
    local hash =  'banned:'..result.to.id
    redis:srem(hash, result.from.id)
  else
    return 'Use This in Your Groups'
  end
end