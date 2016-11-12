
--- Module implementing the LuaRocks "make" command.
-- Builds sources in the current directory, but unlike "build",
-- it does not fetch sources, etc., assuming everything is 
-- available in the current directory.
--module("luarocks.make", package.seeall)
local make = {}
package.loaded["luarocks.make"] = make

local build = require("luarocks.build")
local fs = require("luarocks.fs")
local util = require("luarocks.util")
local cfg = require("luarocks.cfg")
local fetch = require("luarocks.fetch")
local pack = require("luarocks.pack")
local remove = require("luarocks.remove")
local deps = require("luarocks.deps")

make.help_summary = "Compile package in current directory using a rockspec."
make.help_arguments = "[--pack-binary-rock] [<rockspec>]"
make.help = [[
Builds sources in the current directory, but unlike "build",
it does not fetch sources, etc., assuming everything is 
available in the current directory. If no argument is given,
look for a rockspec in the current directory. If more than one
is found, you must specify which to use, through the command-line.

This command is useful as a tool for debugging rockspecs. 
To install rocks, you'll normally want to use the "install" and
"build" commands. See the help on those for details.

--pack-binary-rock  Do not install rock. Instead, produce a .rock file
                    with the contents of compilation in the current
                    directory.

--keep              Do not remove previously installed versions of the
                    rock after installing a new one. This behavior can
                    be made permanent by setting keep_other_versions=true
                    in the configuration file.

--branch=<name>     Override the `source.branch` field in the loaded
                    rockspec. Allows to specify a different branch to 
                    fetch. Particularly for SCM rocks.

]]

--- Driver function for "make" command.
-- @param name string: A local rockspec.
-- @return boolean or (nil, string, exitcode): True if build was successful; nil and an
-- error message otherwise. exitcode is optionally returned.
function make.run(...)
   local flags, rockspec = util.parse_flags(...)
   assert(type(rockspec) == "string" or not rockspec)
   
   if not rockspec then
      for file in fs.dir() do
         if file:match("rockspec$") then
            if rockspec then
               return nil, "Please specify which rockspec file to use."
            else
               rockspec = file
            end
         end
      end
      if not rockspec then
         return nil, "Argument missing: please specify a rockspec to use on current directory."
      end
   end
   if not rockspec:match("rockspec$") then
      return nil, "Invalid argument: 'make' takes a rockspec as a parameter. "..util.see_help("make")
   end

   if flags["pack-binary-rock"] then
      local rspec, err, errcode = fetch.load_rockspec(rockspec)
      if not rspec then
         return nil, err
      end
      return pack.pack_binary_rock(rspec.name, rspec.version, build.build_rockspec, rockspec, false, true, deps.get_deps_mode(flags))
   else
      local ok, err = fs.check_command_permissions(flags)
      if not ok then return nil, err, cfg.errorcodes.PERMISSIONDENIED end
      ok, err = build.build_rockspec(rockspec, false, true, deps.get_deps_mode(flags))
      if not ok then return nil, err end
      local name, version = ok, err
      if (not flags["keep"]) and not cfg.keep_other_versions then
         local ok, err = remove.remove_other_versions(name, version, flags["force"])
         if not ok then util.printerr(err) end
      end
      return name, version
   end
end

return make
