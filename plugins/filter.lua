local function addword(msg, name)
    local hash = 'chat:'..msg.to.id..':badword'
    redis:hset(hash, name, 'newword')
    return "New filter\n>"..name
end

local function get_variables_hash(msg)

    return 'chat:'..msg.to.id..':badword'

end 

-------------------filterlist-------------------
local function list_variablesbad(msg)
  local hash = get_variables_hash(msg)

  if hash then
    local names = redis:hkeys(hash)
    local text = '⛔️<code>FilterList</code>🚫 :\n'
    for i=1, #names do
      text = text..'(<b>'..names[i]..'</b>)\n'
    end
    return text
	else
	return 
  end
end

-------------------cleanfilter----------------
function clear_commandbad(msg, var_name)  
  local hash = get_variables_hash(msg)
  redis:del(hash, var_name)
  return 'Removed'
end

---------------------filterlist------------------
local function list_variables2(msg, value)
  local hash = get_variables_hash(msg)
  
  if hash then
    local names = redis:hkeys(hash)
    local text = ''
    for i=1, #names do
	if string.match(value, names[i]) and not is_momod(msg) then
	if msg.to.type == 'channel' then
	delete_msg(msg.id,ok_cb,false)
	else
	kick_user(msg.from.id, msg.to.id)

	end
return 
end
    end
  end
end

-------------------------get-valuebad--------------------
local function get_valuebad(msg, var_name)
  local hash = get_variables_hash(msg)
  if hash then
    local value = redis:hget(hash, var_name)
    if not value then
      return
    else
      return value
    end
  end
end

----------------------clean-commands------------------------
function clear_commandsbad(msg, cmd_name) 
  local hash = get_variables_hash(msg)
  redis:hdel(hash, cmd_name)
  return ''..cmd_name..'  removed'
end

--------------------run-------------------
local function run(msg, matches)
  if matches[1] == 'filter' and matches[2] == '+' then
  if not is_owner(msg) then 
  return '' 
  end
  local name = string.sub(matches[3], 1, 50)
  local text = addword(msg, name)
  return text
  end
  if matches[1] == 'filterlist' and is_owner(msg) then
  return list_variablesbad(msg)
  end
  if matches[1] == 'clean' and matches[2] == "filterlist" then
  if not is_owner(msg) then 
  return '' 
  end
  local asd = '1'
    return clear_commandbad(msg, asd)
	end
  if matches[1] == 'filter' and matches[2] == '-' then
  if not is_owner(msg) then 
  return '' 
  end
    return clear_commandsbad(msg, matches[3])
  else
    local name = user_print_name(msg.from)
  
    return list_variables2(msg, matches[1])
  end
end


return {
  patterns = {
  "^[!/](filter) (+) (.*)$",
  "^[!/](filter) (-) (.*)$",
    "^[!/](filterlist)$",
    "^[!/](clean) (.*)",
"^(.+)$",
	   
  },
  run = run
}
