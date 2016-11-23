package = "xml"
version = "1.1.3-1"
source = {
  url = 'git://github.com/lubyk/xml',
  tag = 'REL-1.1.3',
  dir = 'xml',
}
description = {
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
  homepage = "http://doc.lubyk.org/xml.html",
  license = "MIT"
}

dependencies = {
  "lua >= 5.1, < 5.4",
  "lub >= 1.0.3, < 2",
}
build = {
  type = 'builtin',
  modules = {
    -- Plain Lua files
    ['xml'            ] = 'xml/init.lua',
    ['xml.Parser'     ] = 'xml/Parser.lua',
    -- C module
    ['xml.core'       ] = {
      sources = {
        'src/Parser.cpp',
        'src/bind/dub/dub.cpp',
        'src/bind/xml_Parser.cpp',
        'src/bind/xml_core.cpp',
      },
      incdirs   = {'include', 'src/bind', 'src/vendor'},
    },
  },
  platforms = {
    linux = {
      modules = {
        ['xml.core'] = {
          sources = {
          },
          libraries = {'stdc++'},
        },
      },
    },
    macosx = {
      modules = {
        ['xml.core'] = {
          sources = {
          },
          libraries = {'stdc++'},
        },
      },
    },
  },
}

