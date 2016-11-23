package="lbase64"
version="20120807-3"
source = {
   url = "http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/5.2/lbase64.tar.gz",
   dir = "base64"
}
description = {
   summary = "A Base64 library for Lua",
   detailed = [[
      A Base64 library for Lua. Base64 is a base-64 transfer encoding that
      uses the characters A-Z, a-z, and 0-9 in that order for the first 62
      digits.
   ]],
   homepage = "http://www.tecgraf.puc-rio.br/~lhf/ftp/lua/#lbase64",
   license = "Public domain",
   maintainer = "Ignacio Burgueño <iburgueno@gmail.com>"
}
dependencies = {
   "lua >= 5.2, < 5.4"
}

build = {
   type = "builtin",
   modules = {
      base64 = "lbase64.c"
   }
}