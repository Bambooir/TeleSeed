do


local function action_by_reply(extra, success, result)
  local user_info = {}
  local uhash = 'user:'..result.from.id
  local user = redis:hgetall(uhash)
  local um_hash = 'msgs:'..result.from.id..':'..result.to.id
  user_info.msgs = tonumber(redis:get(um_hash) or 0)
  user_info.name = user_print_name(user)..' ['..result.from.id..']'
  local msgs = '6-messages sent : '..user_info.msgs
  if result.from.username then
    user_name = '@'..result.from.username
  else
    user_name = ''
  end
  local msg = result
  local user_id = msg.from.id
  local chat_id = msg.to.id
  local user = 'user#id'..msg.from.id
  local chat = 'chat#id'..msg.to.id
  local data = load_data(_config.moderation.data)
  if data[tostring('admins')][tostring(user_id)] then
    who = 'Admim'
  elseif data[tostring(msg.to.id)]['moderators'][tostring(user_id)] then
    who = 'Moderator'
  elseif data[tostring(msg.to.id)]['set_owner'] == tostring(user_id) then
    who = 'Owner'
  elseif tonumber(result.from.id) == tonumber(our_id) then
    who = 'Group creator'
  else
    who = 'Member'
  end
  for v,user in pairs(_config.sudo_users) do
    if user == user_id then
      who = 'Sudo'
    end
  end
  local text = '1-Full name : '..(result.from.first_name or '')..' '..(result.from.last_name or '')..'\n'
             ..'2-First name : '..(result.from.first_name or '')..'\n'
             ..'3-Last name : '..(result.from.last_name or '')..'\n'
             ..'4-Username : '..user_name..'\n'
             ..'5-ID : '..result.from.id..'\n'
             ..msgs..'\n'
             ..'7-Position in group : '..who
  send_large_msg(extra.receiver, text)
end

local function run(msg)
   if msg.text == '!info' and msg.reply_id and is_momod(msg) then
     get_message(msg.reply_id, action_by_reply, {receiver=get_receiver(msg)})
   end
end

return {
    patterns = {
      '^!info$'
    },
  run = run
}
end
