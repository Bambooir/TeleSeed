local function run(msg)
    local data = load_data(_config.moderation.data)
     if data[tostring(msg.to.id)]['settings']['lock_tgservice'] == 'yes' then
delete_msg(msg.id, ok_cb, false)
      end
   end
    
return {
    patterns = {
        "!!tgservice (.*)",
},
run = run
}
