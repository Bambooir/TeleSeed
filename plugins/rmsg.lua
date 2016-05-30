local function history(extra, suc, result)
  for i=1, #result do
    delete_msg(result[i].id, ok_cb, false)
  end
  if tonumber(extra.con) == #result then
    send_msg(extra.chatid, ''..#result..' message has been deleted !', ok_cb, false)
  else
   return
  end
end
local function run(msg, matches)
  if matches[1] == 'rmsg' and is_momod(msg) then
    if msg.to.type == 'channel' then
      if tonumber(matches[2]) > 99 or tonumber(matches[2]) < 1 then
        return "Wrong number, range is [1-99]"
      end
      get_history(msg.to.peer_id, matches[2] + 1 , history , {chatid = msg.to.peer_id, con = matches[2]})
    else
      return "It's only working on Supergroups"
    end
  else
    return 
  end
end

return {
    patterns = {
       '^[!/#](rmsg) (%d*)$'
    },
    run = run
}