
--- Build back-end for raw listing of commands in rockspec files.
--module("luarocks.build.command", package.seeall)
local command = {}

local fs = require("luarocks.fs")
local util = require("luarocks.util")

--- Driver function for the "command" build back-end.
-- @param rockspec table: the loaded rockspec.
-- @return boolean or (nil, string): true if no errors ocurred,
-- nil and an error message otherwise.
function command.run(rockspec)
   assert(type(rockspec) == "table")

   local build = rockspec.build
   
   util.variable_substitutions(build, rockspec.variables)
   
   if build.build_command then
      util.printout(build.build_command)
      if not fs.execute(build.build_command) then
         return nil, "Failed building."
      end
   end
   if build.install_command then
      util.printout(build.install_command)
      if not fs.execute(build.install_command) then
         return nil, "Failed installing."
      end
   end
   return true
end

return command
