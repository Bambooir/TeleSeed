package = "OAuth"
version = "0.0.6-1"
source = {
	url = "https://github.com/ignacio/LuaOAuth/archive/0.0.6-1.tar.gz",
	dir = "LuaOAuth-0.0.6-1"
}
description = {
	summary = "Lua OAuth, an OAuth client library.",
	detailed = [[
		Lua client for OAuth 1.0 enabled servers.
	]],
	license = "MIT/X11",
	homepage = "http://github.com/ignacio/LuaOAuth"
}
dependencies = {
	"lua >= 5.1, <= 5.2",
	"luasocket",
	"luasec",
	"luacrypto",
	"lbase64"
}

external_dependencies = {

}

build = {
	type = "builtin",
	modules = {
		OAuth = "src/OAuth.lua",
		["OAuth.helpers"] = "src/OAuth/helpers.lua",
		["OAuth.coreLuaSocket"] = "src/OAuth/coreLuaSocket.lua",
		["OAuth.coreLuaNode"] = "src/OAuth/coreLuaNode.lua",
	}
}
