do
 local function save_file(name, text)
    local file = io.open("./plugins/"..name, "w")
    file:write(text)
    file:flush()
    file:close()
    return "Plugin saved."
end   
function run(msg, matches)
  if matches[1] == "addplug" and is_sudo(msg) then
 
         local name = matches[2]
        local text = matches[3]
        return save_file(name, text)
        
  end
end
return {
  patterns = {
  "^/(addplug) ([^%s]+) (.+)$"
  },
  run = run
}
end
-- create by @shervin_hacker
