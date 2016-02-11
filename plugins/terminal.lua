function run_sh(msg, matches)
     name = get_name(msg)
     text = ''
     -- if config.sh_enabled == false then 
        -- text = '!sh command is disabled'
     -- else
        -- if is_sudo(msg) then
           -- bash = msg.text:sub(4,-1)
           -- text = run_bash(bash)
        -- else
           -- text = name .. ' you have no power here!'
        -- end
     -- end
  if is_sudo(msg) then
        bash = msg.text:sub(4,-1)
        text = run_bash(bash)
     else
        text = name .. ' you have no power here!'
     end
     return text
end

function run_bash(str)
    local cmd = io.popen(str)
    local result = cmd:read('*all')
    cmd:close()
    return result
end

function on_getting_dialogs(cb_extra,success,result)
  if success then
    local dialogs={}
    for key,value in pairs(result) do 
      for chatkey, chat in pairs(value.peer) do
        print(chatkey,chat)
        if chatkey=="id" then
          table.insert(dialogs,chat.."\n")
        end
        if chatkey=="print_name" then
          table.insert(dialogs,chat..": ")
        end
      end 
    end

    send_msg(cb_extra[1],table.concat(dialogs),ok_cb,false)
  end
end
function run(msg, matches)
  if not is_sudo(msg) then
    return "You aren't allowed!"
  end
  local receiver = get_receiver(msg)
 if string.match then
   text = run_bash(matches[1])
    send_large_msg(receiver, text, ok_cb, false)
    return
  end
 end
return {
    description = "shows cpuinfo", 
    usage = "!cpu",
    patterns = {"^[Tt]r (.*)$"}, 
    run = run 
}
