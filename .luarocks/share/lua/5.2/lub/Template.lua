--[[------------------------------------------------------

  # Simple templating

  Simplistic templating system inspired by Zed A. Shaw's
  minimal template for [Tir](http://mongrel2.org/).

  Template features:

  + `{{ code }}`  :    Replaced with the string provided by 'code'.
  + `{% code %}`  :    Execute code but do not output (used for loops, if, etc).
  + `{| code |}`  :    Output code and preserve indentation.

  Usage:

    local tmplt = lub.Template [%%[
    Hello {{name}}.
    {% if parent then %}
    Your parent is {{parent.name}}.
    {% end %}
    ]%%]

    tmplt:run {
      name = 'Joe',
      parent = {name = 'Eve'},
    }
    --> Hello Joe.
    --> Your parent is Eve.
--]]------------------------------------------------------
local lub     = require 'lub'
local lib     = lub.class 'lub.Template'
local NO_FENV = not rawget(_G, 'setfenv')
local private = {}

local loadstring = loadstring or load

-- Create a new template object ready to produce output by calling #run. The
-- `source` parameters should be a string or a table with a `path` key.
function lib.new(source)
  local self
  if type(source) == 'string' then
    self = {source = source}
  else
    self = assert(source)
    -- Grab source from path...
    local file = assert(io.open(self.path, 'r'))
    self.source = file:read('*a')
    file:close()
  end
  setmetatable(self, lib)
  private.makeFunction(self, self.source)
  return self
end

-- Returns resulting text by runngint the template with an environment table
-- `env`.
function lib:run(env)
  env = env or {}
  local buffer_ = ''
  function env._out_(str)
    buffer_ = buffer_ .. (str or '')
  end
  function env._indout_(str, indent)
    buffer_ = buffer_ .. indent .. string.gsub(str, '\n', indent)
  end
  setmetatable(env, {__index = _G})
  local ok, err = xpcall(function() self.func(env) end, debug.traceback)
  if not ok then
    local _, line
    _, _, line = string.find(err, ":(%d+): in function 'func'")
    if not line then
      error(err)
    end
    line = line * 1
    err = lub.split(err, '\n')[1]
    print(err)
    print('-------------------------------------')
    print('Error: ', err)
    local lines = lub.split(self.lua, '\n')
    for l = line-6, line+6 do
      if l == line then
        print(lines[l], '<================')
      else
        print(lines[l])
      end
    end
    
    error(lines[line])
  end
  return buffer_
end

function private:makeFunction(source)
  local res = 'local function _fun_(env)\n'
  if NO_FENV then
    res = res..'  local _ENV = env\n'
  else
    res = res..'  setfenv(1, env)\n'
  end
  self.lua = res..private.parse(self, source)..'\nend\nreturn _fun_'
  self.func, self.err = loadstring(self.lua)
  if self.err then
    print(self.lua)
    assert(false, self.err)
  end
  -- We compile as a function so that we can pass 'env' as parameter.
  self.func = self.func()
end  
  
-- (internal). Return Lua code from the template string. The generated code
-- outputs content by calling `env._out_` function.
function private:parse(source)
  local res = ''
  local indout_fmt, out_fmt
  local str_join = ''
  while source:find(']'..str_join..']') do
    str_join = str_join .. '='
  end
  indout_fmt = '_indout_(%s, ['..str_join..'[%s]'..str_join..'])\n'
  out_fmt    = '_out_(['..str_join..'[%s]'..str_join..'])\n'

  local eat_next_newline
  -- Find balanced {
  for text, block in string.gmatch(source .. '{{}}', '([^{]-)(%b{})') do
    if text ~= '' then
      if string.sub(text, 1, 1) == "\n" then
        if not eat_next_newline then
          -- Avoid multiline return removal
          text = "\n" .. text
        end
      end
    end
    -- handle block
    eat_next_newline = false
    local block_type = string.sub(block, 1, 2)
    local content = string.sub(block, 3, -3)
    local block_text = ''
    if block_type == '{{' then
      -- output content
      if content ~= '' then
        block_text = string.format("_out_(%s)\n", content)
      end
    elseif block_type == '{|' then
      -- output content with indentation
      if content ~= '' then
        block_text = string.format(indout_fmt, content, text)
        text = nil
      end
    elseif block_type == '{%' then
      block_text = content .. "\n"
      eat_next_newline = true
    else
      text = text .. '{'
      block_text = private.parse(self, string.sub(block, 2, -1))
    end
    if text and text ~= '' then
      res = res .. string.format(out_fmt, text) .. block_text
    else
      res = res .. block_text
    end
  end
  return res
end

return lib
