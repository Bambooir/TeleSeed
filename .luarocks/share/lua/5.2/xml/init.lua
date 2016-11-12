--[[------------------------------------------------------
  # Very fast xml parser for Lua <a href="https://travis-ci.org/lubyk/xml"><img src="https://travis-ci.org/lubyk/xml.png" alt="Build Status"></a> 

  This parser uses [RapidXML](http://rapidxml.sourceforge.net/) to parse XML
  content.

  <html><a href="https://github.com/lubyk/xml"><img style="position: absolute; top: 0; right: 0; border: 0;" src="https://s3.amazonaws.com/github/ribbons/forkme_right_green_007200.png" alt="Fork me on GitHub"></a></html>

  *MIT license* &copy Marcin Kalicinski 2006, 2009, Gaspard Bucher 2014.

  ## Installation
  
  With [luarocks](http://luarocks.org):

    $ luarocks install xml
  
  ## Usage example

    local data = xml.load(some_xml)

    local xml_string = xml.dump(some_table)

--]]-----------------------------------------------------
local lub     = require 'lub'
local lib     = lub.Autoload 'xml'
local ipairs, pairs, insert, type,
      match, tostring =
      ipairs, pairs, table.insert, type,
      string.match, tostring

local parser  = lib.Parser()


-- Current version respecting [semantic versioning](http://semver.org).
lib.VERSION = '1.1.2'

lib.DEPENDS = { -- doc
  -- Compatible with Lua 5.1 to 5.3 and LuaJIT
  'lua >= 5.1, < 5.4',
  -- Uses [Lubyk base library](http://doc.lubyk.org/lub.html)
  'lub >= 1.0.3, < 2',
}

-- nodoc
lib.DESCRIPTION = {
  summary = "Very fast xml parser based on RapidXML",
  detailed = [[
    This module is part of the Lubyk project.

    Main features are:
     - Fast and easy to use
     - Complete documentation
     - Based on proven code (RapidXML)
     - Full test coverage

    Read the documentation at http://doc.lubyk.org/xml.html.
  ]],
  homepage = "http://doc.lubyk.org/"..lib.type..".html",
  author   = "Gaspard Bucher",
  license  = "MIT",
}

-- nodoc
lib.BUILD = {
  github    = 'lubyk',
  includes  = {'include', 'src/bind', 'src/vendor'},
  platlibs = {
    linux  = {'stdc++'},
    macosx = {'stdc++'},
  },
  -- FIXME: Implement platform flags for lut.Builder and see how it works with
  -- luarocks /EHsc is needed for exception handling.
  platflags = {
    win32 = {'EHsc'},
  },
}

--[[

  # Lua table format

  This xml library uses string keys in Lua tables to store
  attributes and numerical keys for sub-nodes. Since the 'xml'
  attribute is not allowed in XML, we use this key to store the
  tag. Here is an example of Lua content:

  ## Lua

    {xml='document',
      {xml = 'article',
        {xml = 'p', 'This is the first paragraph.'},
        {xml = 'h2', class = 'opt', 'Title with opt style'},
      },
      {xml = 'article',
        {xml = 'p', 'Some ', {xml = 'b', 'important'}, ' text.'},
      },
    }

  ## XML

  And the equivalent xml:

    #txt
    <document>
      <article>
        <p>This is the first paragraph.</p>
        <h2 class='opt'>Title with opt style</h2>
      </article>
      <article>
        <p>Some <b>important</b> text.</p>
      </article>
    </document>

  # Notes on speed

  RapidXML is a very fast parser that uses in-place modification of the input
  text. Since Lua strings are immutable, we have to make a copy except for the
  xml.Parser.NonDestructive and xml.Parser.Fastest settings. With these types
  some xml entities such as `&amp;lt;` are not translated.

  See [RapidXML](http://rapidxml.sourceforge.net/) for details.
--]]------------------------------------------------------

-- # Class methods

-- Parse a `string` containing xml content and return a table. Uses
-- xml.Parser with the xml.Parser.Default type.
function lib.load(string)
  return parser:load(string)
end

-- Parse the XML content of the file at `path` and return a lua table. Uses
-- xml.Parser with the xml.Parser.Default type.
function lib.loadpath(path)
  return parser:load(lub.content(path))
end

local function escape(v)
  if type(v) == 'boolean' then
    return v and 'true' or 'false'
  else
    return v:gsub('&','&amp;'):gsub('>','&gt;'):gsub('<','&lt;'):gsub("'",'&apos;')
  end
end

local function tagWithAttributes(data)
  local res = data.xml or 'table'
  for k,v in pairs(data) do
    if k ~= 'xml' and type(k) == 'string' then
      res = res .. ' ' .. k .. "='" .. escape(v) .. "'"
    end
  end
  return res
end

local function doDump(data, indent, output, last, depth, max_depth)
  if depth > max_depth then
    error(string.format("Could not dump table to XML. Maximal depth of %i reached.", max_depth))
  end

  if data[1] then
    insert(output, (last == 'n' and indent or '')..'<'..tagWithAttributes(data)..'>')
    last = 'n'
    local ind = indent..'  '
    for _, child in ipairs(data) do
      local typ = type(child)
      if typ == 'table' then
        doDump(child, ind, output, last, depth + 1, max_depth)
        last = 'n'
      elseif typ == 'number' then
        insert(output, tostring(child))
      else
        local s = escape(child)
        insert(output, s)
        last = 's'
      end
    end
    insert(output, (last == 'n' and indent or '')..'</'..(data.xml or 'table')..'>')
    last = 'n'
  else
    -- no children
    insert(output, (last == 'n' and indent or '')..'<'..tagWithAttributes(data)..'/>')
    last = 'n'
  end
end

-- Dump a lua table in the format described above and return an XML string. The
-- `max_depth` parameter is used to avoid infinite recursion in case a table
-- references one of its ancestors.
--
-- Default maximal depth is 3000.
function lib.dump(table, max_depth)
  local max_depth = max_depth or 3000
  local res = {}
  doDump(table, '\n', res, 's', 1, max_depth)
  return lub.join(res, '')
end


local function doRemoveNamespace(data, prefix)
  data.xml = match(data.xml, prefix .. ':(.*)') or data.xml
  for _, sub in ipairs(data) do
    if type(sub) == 'table' then
      doRemoveNamespace(sub, prefix)
    end
  end
end

-- This function finds the `xmlns:NAME='KEY'` declaration and removes `NAME:` from
-- the tag names.
--
-- Example:
--
--   local data = xml.load [[
--    <foo:document xmlns:foo='bar'>
--      <foo:name>Blah</foo:name>
--    </foo:document>
--   ]]
--   
--   xml.removeNamespace(data, 'bar')
--
--   -- Result
--   {xml = 'document', ['xmlns:foo'] = 'bar',
--     {xml = 'name', 'Blah'},
--   }
function lib.removeNamespace(data, key)
  local nm
  for k, v in pairs(data) do
    if v == key then
      nm = match(k, 'xmlns:(.*)')
      if nm == '' then
        -- error
        return
      else
        doRemoveNamespace(data, nm)
      end
    end
  end
end

-- Recursively find the first table with a tag equal to `tag`. This
-- search uses [lub.search](lub.html#search) to do an [Iterative deepening depth-first search](https://en.wikipedia.org/wiki/Iterative_deepening_depth-first_search)
-- because we usually search for elements close to the surface.
--
-- For more options, use [lub.search](lub.html#search) directly with a custom
-- function.
--
-- You can also pass an attribute key and attribute value to further filter the
-- searched node. This gives this function the same arguments as LuaXML's find
-- function.
--
-- Usage examples:
--
--   local sect = xml.find(elem, 'simplesect', 'kind', 'section')
--
--   print(xml.find(sect, 'title'))
function lib.find(data, tag, attr_key, attr_value)
  if attr_key then
    return lub.search(data, function(node)
      if node.xml == tag and node[attr_key] == attr_value then
        return node
      end
    end)
  else
    return lub.search(data, function(node)
      if node.xml == tag then
        return node
      end
    end)
  end
end

-- # Classes

return lib
