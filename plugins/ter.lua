do

function run(msg, matches)
  if is_sudo(msg) then
		local text = io.popen(matches[1]):read('*all')
		reply_msg(msg.id, text, ok_cb, false)
		end
  if not is_sudo(msg) then
		return 
	end
end

return {
  description = 'Run a system command.',
  usage = {
    sudo = {
      '$<command>\n'
      ..'\nWARNING:'
      ..'\nThis plugin interfaces with your operating system.'
      ..'\nEven without root privileges, a bad command can be harmful.\n'
      ..'\nOnly for sudoers. Turned off after use.',
    },
  },
  patterns = {
    '^$(.*)$'
  },
  run = run,
  moderated = true
}

end
