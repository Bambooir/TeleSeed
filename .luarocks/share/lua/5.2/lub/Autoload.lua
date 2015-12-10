--[[------------------------------------------------------

  # Autoload

  This class creates a module table that will automatically call require to
  find new keys. This enables "load as needed" code. If the required code
  cannot be found, generates an error.
  
--]]------------------------------------------------------
local lub  = require 'lub'
local lib  = lub.class 'lub.Autoload'
local lib2 = {}
local private = {}

-- Declare a new table with autoloading feature. The `prefix` should reflect
-- the path used to call require for missing elements. If the optional `tbl`
-- is provided, this table will be used as starting content.
function lib.new(prefix, tbl)
  assert(prefix, 'Missing prefix in Autoload call.')
  local self = tbl or {}
  self.type = prefix
  return setmetatable(self, lib)
end

-- nodoc
function lib.__index(table, name)
  -- Trying to load new lib like foo.Bar
  local ok, new_lib_or_error = pcall(function() return require(table.type..'.'..name); end)
  if ok then
    -- insert new class with name in table
    table[name] = new_lib_or_error
    return new_lib_or_error
  else
    print(new_lib_or_error)
    return nil
  end
end

-- # Global
-- You can enable autoload on the global namespace `_G` to require modules.
--   
-- WARN: You should not disable #strict or `nil` values in global namespace will
-- have a *huge* overhead (try to require code on every access).


-- Activate autoloading for global environment `_G` (this method is slightly
-- different then #new and adapted for the global environment). By default,
-- this uses #strict (forbids using global nil values).
function lib.global()
  setmetatable(_G, lib2)
end

-- This can be used to disable strict mode when autoload is enabled for global
-- environment.
function lib.strict(x)
  if x == false then
    -- not strict
    lib2.__index = private.__index
  else
    -- disable autoload and raise on nil
    lib2.__index = private.__index_strict
  end
end

--- Autoload for _G
function private.__index(table, name)
  if name == '_PROMPT' or name == '_PROMPT2' then
    -- special case for lua interactive
    return nil
  end
  local ok, new_lib_or_error = pcall(function() return require(name); end)
  if ok then
    return new_lib_or_error or rawget(table, name)
  else
    -- nil value
    return nil
  end
end

--- Autoload for _G (strict mode = no nil allowed)
function private.__index_strict(table, name)
  if name == '_PROMPT' or name == '_PROMPT2' then
    -- special case for lua interactive
    return nil
  end
  local ok, new_lib_or_error = pcall(function() return require(name); end)
  if ok then
    return new_lib_or_error or rawget(table, name)
  else
    -- nil value
    error(string.format("Trying to access undefined global variable '%s'.\n%s", name, debug.traceback()))
  end
end

-- Default is strict.
lib2.__index = private.__index_strict

return lib
