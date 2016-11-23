--[[------------------------------------------------------
  # Parser object
  
  The parser class is used to encapsulate parsing and settings in an object. When
  using default settings, it is not necessary to create parser objects and
  one can simply use [xml.load](xml.html#load).

--]]------------------------------------------------------
local core = require 'xml.core'
local lub  = require 'lub'
local lib  = core.Parser

-- ## Parser types
-- The following constants can be used with #new when creating a parser.
-- Depending on the type, the parser behavior is different. The default type is
-- Default.

-- Default parser type. Translates xml entities (needs to make an internal
-- copy of the lua string).
-- lib.Default

-- Same as default but trims leading and trailing whitespace.
-- lib.TrimWhitespace

-- Faster then default, does not translate xml entities. Parses data nodes.
-- lib.NonDestructive

-- Like xml.NonDestructive but does not parse data nodes.
-- lib.Fastest

-- Create a new parser. `type` flag is optional. If you are using the default
-- parser, you can simply use [xml.load](xml.html#load).
-- Usage example:
--
--   local xml = require 'xml'
--   local parser = xml.Parser(xml.Parser.Fastest)
-- function lib.new(type)

-- Parse an xml string and return a Lua table. See [lua/xml](xml.html)
-- for the format of the returned table. Usage:
--
--   local data = parser:load(some_xml_string)
--   --> lua table
-- function lib:load(str)

-- Parse xml from file `path` and return a Lua table. See [lua/xml](xml.html)
-- for the format of the returned table. Usage:
--
--   local data = parser:loadpath(path_to_file)
--   --> lua table
function lib:loadpath(path)
  return self:load(lub.content(path))
end

return lib
