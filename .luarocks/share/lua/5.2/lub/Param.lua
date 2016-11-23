--[[------------------------------------------------------

  # Parameter save and restore

  This class acts as a proxy around lua tables allowing
  paramter save and recall.

--]]------------------------------------------------------
local lub  = require 'lub'
local yaml = require 'yaml'

-- We keep table keys sorted to improve settings file stability (useful with
-- versioning systems).
yaml.configure {
  sort_table_keys = true,
}

local lib = lub.class('lub.Param')

-- Metatable for proxies.
local ProxyMt = {}

-- Metatable for controllers.
local CtrlMt  = {}

local private = {}

-- ## Dependencies
--
-- * yaml

-- # Class functions

-- Create a new parameter helper saving content to `filepath`. If the file path
-- is not provided, the default is to use the current script name with ".yml"
-- extension. For example "foo.lua" parameters would be saved to "foo.lua.yml".
function lib.new(filepath)
  if not filepath then
    filepath = lub.path('&', 3)
    if not filepath then
      -- This can happen with tail call optimization
      filepath = lub.path('&', 4)
    end
    filepath = filepath..'.yml'
  end

  local self  = {
    preset    = 'p1',
    presets   = {},
    filepath  = filepath,
    proxies   = {},
    controls  = {},
    mappings  = {},
    rmappings = {},
  }
  setmetatable(self, lib)
  private.loadpath(self, self.filepath)
  return self
end

-- # Methods

-- Create a proxy table to write to instead of the original. Writing to the
-- proxy stores the parameter value and write the value into the original table.
-- Since a single Param object can store values from different tables, the
-- `proxy_name` string is used to separate values during save/restore. Default
-- value for `proxy_name` is 'main'.
function lib:proxy(original_table, proxy_name, settings)
  local settings = settings or {}
  local proxy = setmetatable({
    __param     = self,
    __storage   = {},
    __original  = original_table,
    __name      = proxy_name    or 'main',
  }, ProxyMt)
  self.proxies[proxy.__name] = proxy
  local proxy_data = (self.presets[self.preset] or {})[proxy.__name]
  if proxy_data then
    for key, value in pairs(proxy_data) do
      self:setValue(proxy, key, value)
    end
  end
  return proxy
end

-- nodoc
function lib:setValue(proxy, key, value, controller)
  local storage = proxy.__storage
  local old_value = storage[key]
  -- Always push value to original, even if it hasn't changed (it may change in
  -- original without us knowing it).
  proxy.__original[key] = value

  -- only map or notify if parameter is CHANGED
  if old_value ~= value then
    -- Cache value so that we can write preset to file.
    rawset(storage, key, value)
    if self.learn_next then
      self.lchange = {proxy_name = proxy.__name, key = key}
      self.learn_next = false
    end
    private.notifyControls(self, proxy.__name, key, value, controller)
  end
end

-- nodoc
-- `value` is already mapped to [0,1].
function lib:control(ctrl, key, value)
  -- get mapping
  local rmapping = (self.rmappings[ctrl.name] or {})
  if not rmapping then return end

  local rmap = rmapping[key]
  if not rmap then return end

  local proxy = self.proxies[rmap.proxy_name]
  if not rmap then return end

  self:setValue(proxy, rmap.key, value, ctrl)
end

-- Map parameter. If `proxy_name` and `key` are omitted, the last changed
-- parameter is used instead.
function lib:mapParameter(ctrl_name, ctrl_key, proxy_name, key)

  if not proxy_name then
    local lchange = self.lchange
    if not lchange then
      print("missing 'last changed' parameter. Cannot learn mapping.")
      return
    end

    proxy_name = lchange.proxy_name
    key        = lchange.key
  end

  -- ctrl_name = 'mi'
  -- ctrl_key  = 44
  -- proxy_name = 'main'
  -- key = 'hello'

  -- forward mapping
  -- main:
  --   hello:
  --     mi: 44
  lub.deepMerge(self.mappings, proxy_name, {
    [key] = {
      [ctrl_name] = ctrl_key
    }
  })

  -- reverse mapping
  -- mi:
  --   44:
  --     proxy_name: main
  --     key: hello
  lub.deepMerge(self.rmappings, ctrl_name, {
    [ctrl_key] = {
      proxy_name = proxy_name,
      key        = key,
    }
  })

  private.save(self)
end

-- Serialize all preset values to yaml.
function lib:dump()
  return yaml.dump({
    preset   = self.preset,
    presets  = self.presets,
    mappings = self.mappings,
  })
end

-- Save current table values in current preset.
function lib:savePreset()
  local preset = self.presets[self.preset]
  if not preset then
    preset = {}
    self.presets[self.preset] = preset
  end
  for proxy_name, proxy in pairs(self.proxies) do
    local tbl = preset[proxy_name]
    if not tbl then
      tbl = {}
      preset[proxy_name] = tbl
    end
    for k, value in pairs(proxy.__storage) do
      tbl[k] = value
    end
  end

  -- Write to file
  private.save(self)
end

-- Select preset named `preset_name` and load all values defined in preset into
-- original tables. Values not defined in the preset are not removed and will be
-- saved with preset.
function lib:selectPreset(preset_name)
  if not preset_name then
    print('Cannot use nil as preset name !')
    return
  end
  if type(preset_name) == 'number' then
    preset_name = 'p'..preset_name
  end
  local preset_data = self.presets[preset_name]
  if preset_data then
    -- Change values defined in preset_data
    for proxy_name, proxy_data in pairs(preset_data) do
      local proxy = self.proxies[proxy_name]
      if proxy then
        for key, value in pairs(proxy_data) do
          self:setValue(proxy, key, value)
        end
      end
    end

  else
    -- New preset with same values as current values
  end
  self.preset = preset_name
end

-- Copy all currently defined values to `preset_name`. If the preset already
-- exists, it is replaced.
function lib:copyToPreset(preset_name)
  if type(preset_name) == 'number' then
    preset_name = 'p'..preset_name
  end
  self.preset = preset_name
  self:savePreset()
end


-- # Controller methods

-- Add a new controller. The optional settings table can contain the following
-- keys:
-- + min: minimal value set by controller (0)
-- + max: maximal value set by controller (1)
function lib:addController(name, settings)
  local ctrl = {
    name       = name,
    min        = settings.min or 0,
    max        = settings.max or 1,
    param      = self,
    learn_next = false,
  }
  ctrl.range = ctrl.max - ctrl.min
  assert(ctrl.range ~= 0, 'min and max values should be different')
  self.controls[name] = ctrl
  return setmetatable(ctrl, CtrlMt)
end

-- ================================================== Proxy metatable

--- Proxy metatable
function ProxyMt.__index(proxy, key)
  return proxy.__original[key]
end

function ProxyMt.__newindex(proxy, key, value)
  proxy.__param:setValue(proxy, key, value)
end

-- ================================================== Controller metatable

CtrlMt.__index = CtrlMt

function CtrlMt:learn()
  self.learn_next = true
  self.param.learn_next = true
end

function CtrlMt:__call(key, cvalue)
  local param = self.param
  if self.learn_next then
    param:mapParameter(self.name, key)
    self.learn_next = false
  end

  local value = (cvalue - self.min) / self.range
  param:control(self, key, value)
end

-- Control callback for feedback.
-- function CtrlMt:changed(key, cvalue)
  
-- ================================================== PRIVATE

-- Load presets and mappings from files system
function private:loadpath(filepath)
  if lub.exist(filepath) then
    local data     = yaml.loadpath(filepath)
    self.presets   = data.presets or {}
    self.mappings  = data.mappings or {}
    self.rmappings = {}
    local rmappings = self.rmappings

    -- mappings = 
    -- main:
    --   hello:
    --     mi: 44
    --
    -- rmappings = 
    -- mi:
    --   44: 
    --     proxy_name: main
    --     key: hello
    for proxy_name, proxy_mappings in pairs(self.mappings) do
      for key, mappings in pairs(proxy_mappings) do
        for ctrl_name, ctrl_key in pairs(mappings) do
          -- rmappings.mi = {[44] = {proxy_name = 'main', key = 'one'}}
          lub.deepMerge(rmappings, ctrl_name, {
            [ctrl_key] = {
              proxy_name = proxy_name,
              key = key,
            }
          })
        end
      end
    end

    if data.preset then
      self:selectPreset(data.preset)
    end
  end
end

function private:save()
  lub.writeall(self.filepath, self:dump())
end

function private:notifyControls(proxy_name, key, value, skip_control)
  local proxy_mappings = self.mappings[proxy_name]
  if proxy_mappings then
    local mappings = proxy_mappings[key]
    if mappings then
      local controls = self.controls
      for ctrl_name, ctrl_key in pairs(mappings) do
        local ctrl = controls[ctrl_name]
        if ctrl and ctrl ~= skip_control then
          local fun = ctrl.changed
          if fun then
            fun(ctrl, ctrl_key, ctrl.min + (value * ctrl.range))
          end
        else
          -- print("Missing control '"..ctrl_name.."'.")
        end
      end
    end
  end
end

return lib
