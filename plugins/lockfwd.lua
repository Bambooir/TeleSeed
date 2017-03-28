do

local function pre_process(msg)
    
    local hash = 'mate:'..msg.to.id
    if redis:get(hash) and msg.fwd_from and not is_momod(msg)  then
	  chat_del_user('chat#id'..msg.to.id,'user#id'..msg.from.id, ok_cb, false)
            return "fwd was locked"
        end
    
        return msg
    end

  


local function run(msg, matches)
    chat_id = msg.to.id
    
    if matches[1] == 'lock' and is_momod(msg) then
      
            
                    local hash = 'mate:'..msg.to.id
                    redis:set(hash, true)
                    return "قفل فروارد فعال شد"
  elseif matches[1] == 'unlock' and is_momod(msg) then
                    local hash = 'mate:'..msg.to.id
                    redis:del(hash)
                    return "قفل فروارد غیر فعال شد"
					end
					if matches[1] == 'status' then
                    local hash = 'mate:'..msg.to.id
                    if redis:get(hash) then
                    return "fwd is locked"
					else 
					return "fwd is not locked"

end
end
end
return {
    patterns = {
        '^[/!#](lock) fwd$',
        '^[/!#](unlock) fwd$',
		'^[/!#]fwd (status)$',
    },
    run = run,
    pre_process = pre_process
}
end