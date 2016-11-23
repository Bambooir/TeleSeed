local function run(msg, matches)
    if not permissions(msg.from.id, msg.to.id, "settings") then
        local hash = 'links:'..msg.to.id
        if redis:get(hash) then
            delete_msg(msg.id, ok_cb, false)
            send_report(msg)
        end
    end
end

return {
patterns = {
    -- Agrega mas links si es necesario
    "[Hh][Tt][Tt][Pp][Ss][:][/][/]",
    "[Hh][Tt][Tt][Pp][:][/][/]",
    "[Ww][Ww][Ww][.]",
    "[.][Cc][Oo][Mm]",
    "https?://[%w-_%.%?%.:/%+=&]+%"
}, run = run}
