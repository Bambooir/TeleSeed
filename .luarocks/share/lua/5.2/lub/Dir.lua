--[[------------------------------------------------------

  # Directory helper

  This is a helper to access/glob directory trees. It requires
  'lfs' (lua filesystem).

--]]------------------------------------------------------
local lub = require 'lub'
local lfs = require 'lfs'
local match,        create,           resume,           yield,           huge =
      string.match, coroutine.create, coroutine.resume, coroutine.yield, math.huge

local lib = lub.class('lk.Dir', {
  sep = '/',
  ignore_pattern = '^[.]'
})

-- ## Dependencies
--
-- * lfs

-- # Class functions

-- Create a new directory helper pointing at `path`.
function lib.new(path)
  local self = {path = path}
  return setmetatable(self, lib)
end

local function glob_list(base, pattern, max_depth)
  for file in lfs.dir(base) do
    if not match(file, lib.ignore_pattern) then
      local fullpath = base..lib.sep..file
      local attrs = lfs.attributes(fullpath)
      if attrs and attrs.mode == 'file' then
        if not pattern or match(fullpath, pattern) then
          yield(fullpath)
        end
      elseif attrs and attrs.mode == 'directory' and max_depth > 0 then
        glob_list(fullpath, pattern, max_depth - 1)
      end
    end
  end
end

-- # Methods

-- Return an iterator to recursively find files matching `pattern` in the
-- directory. The pattern syntax is the same as string.match. Recursivity can
-- be altered by setting `max_depth` argument. 0 = do not enter sub-directories.
-- Default value for max_depth is math.huge.
--
--   -- Find paths ending in ".lua".
--   for path in lub.Dir('lub'):glob '%.lua$' do
--     print(path)
--   end
--   --> lub/Dir.lua
--   --> lub/Doc.lua
--   --> ...
function lib:glob(pattern, max_depth)
  local max_depth = max_depth or huge
  local co = create(glob_list)
  return function()
    local ok, value = resume(co, self.path, pattern, max_depth)
    if ok then
      return value
    else
      return nil
    end
  end
end

local function list_files(self)
  local base = self.path
  for file in lfs.dir(base) do
    if not match(file, self.ignore_pattern) then
      yield(base..self.sep..file)
    end
	end
end

-- Return an iterator over the paths in the directory. The returned values are
-- paths, not just filenames.
--
--   for file in lk.Dir('lib'):list() do
--     print(file)
--   end
--   --> lib/lk
--   --> lib/lk.lua
function lib:list()
  local co = create(list_files)
  return function()
    local ok, value = resume(co, self)
    if ok then
      return value
    else
      return nil
    end
  end
end

-- Return true if there is at least one child in the directory that matches
-- `pattern`.
function lib:contains(pattern)
  for file in lfs.dir(self.path) do
    if match(file, pattern) then
      return true
    end
	end
  return false
end

return lib
