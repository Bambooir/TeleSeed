package = "LuaExpat"
version = "1.3.0-1"
source = {
   url = "http://matthewwild.co.uk/projects/luaexpat/luaexpat-1.3.0.tar.gz"
}
description = {
   summary = "XML Expat parsing",
   detailed = [[
      LuaExpat is a SAX (Simple API for XML) XML parser based on the
      Expat library.
   ]],
   license = "MIT/X11",
   homepage = "http://www.keplerproject.org/luaexpat/"
}
dependencies = {
   "lua >= 5.1"
}
external_dependencies = {
   EXPAT = {
      header = "expat.h"
   }
}
build = {
   type = "builtin",
   modules = {
    lxp = { 
      sources = { "src/lxplib.c" },
      libraries = { "expat" },
      incdirs = { "$(EXPAT_INCDIR)", "src/" },
      libdirs = { "$(EXPAT_LIBDIR)" }
    },
    ["lxp.lom"] = "src/lxp/lom.lua"
   },
   copy_directories = { "doc", "tests" }
}
