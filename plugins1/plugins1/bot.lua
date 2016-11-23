--------------------------------------------------
--      ____  ____ _____                        --
--     |    \|  _ )_   _|___ ____   __  __      --
--     | |_  )  _ \ | |/ ·__|  _ \_|  \/  |     --
--     |____/|____/ |_|\____/\_____|_/\/\_|     --
--                                              --
--------------------------------------------------
--                                              --
--       Developers: @Josepdal & @MaSkAoS       --
--     Support: @Skneos,  @iicc1 & @serx666     --
--              @h3iran :D                      --
--------------------------------------------------
local function is_channel_disabled( receiver )
	if not _config.disabled_channels then
		return false
	end

	if _config.disabled_channels[receiver] == nil then
		return false
	end

  return _config.disabled_channels[receiver]
end

local function enable_channel(receiver, to_id)
	if not _config.disabled_channels then
		_config.disabled_channels = {}
	end

	if _config.disabled_channels[receiver] == nil then
		return lang_text(to_id, 'botOn')..' 😏'
	end
	
	_config.disabled_channels[receiver] = false

	save_config()
	return lang_text(to_id, 'botOn')..' 😏'
end

local function disable_channel(receiver, to_id)
	if not _config.disabled_channels then
		_config.disabled_channels = {}
	end
	
	_config.disabled_channels[receiver] = true

	save_config()
	return lang_text(to_id, 'botOff')..' 🚀'
end

local function pre_process(msg)
	local receiver = get_receiver(msg)
	
	-- If sender is sudo then re-enable the channel
	if is_sudo(msg) then
	  if msg.text == "#bot on" or msg.text == "!bot on" or msg.text == "/bot on" then
	    enable_channel(receiver, msg.to.id)
	  end
	end

  if is_channel_disabled(receiver) then
  	msg.text = ""
  end

	return msg
end

local function run(msg, matches)
	if permissions(msg.from.id, msg.to.id, "bot") then
		local receiver = get_receiver(msg)
		-- Enable a channel
		if matches[1] == 'on' then
			return enable_channel(receiver, msg.to.id)
		end
		-- Disable a channel
		if matches[1] == 'off' then
			return disable_channel(receiver, msg.to.id)
		end
	else
		return '🚫 '..lang_text(msg.to.id, 'require_sudo')
	end
end

return {
	patterns = {
		"^[!/#]bot? (on)",
		"^[!/#]bot? (off)" }, 
	run = run,
	pre_process = pre_process
}
