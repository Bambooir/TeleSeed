local site_config = {}
site_config.LUAROCKS_PREFIX=[[/root/Clannadbot/.luarocks]]
site_config.LUA_INCDIR=[[/usr/include/lua5.2]]
site_config.LUA_LIBDIR=[[/usr/lib]]
site_config.LUA_BINDIR=[[/usr/bin]]
site_config.LUA_INTERPRETER=[[lua5.2]]
site_config.LUAROCKS_SYSCONFDIR=[[/root/Clannadbot/.luarocks/luarocks]]
site_config.LUAROCKS_ROCKS_TREE=[[/root/Clannadbot/.luarocks]]
site_config.LUAROCKS_FORCE_CONFIG=true
site_config.LUAROCKS_ROCKS_SUBDIR=[[/lib/luarocks/rocks]]
site_config.LUAROCKS_UNAME_S=[[Linux]]
site_config.LUAROCKS_UNAME_M=[[armv8l]]
site_config.LUAROCKS_DOWNLOADER=[[curl]]
site_config.LUAROCKS_MD5CHECKER=[[md5sum]]
site_config.LUAROCKS_EXTERNAL_DEPS_SUBDIRS={ bin="bin", lib={ "lib", [[lib/arm-linux-gnueabihf]] }, include="include" }
site_config.LUAROCKS_RUNTIME_EXTERNAL_DEPS_SUBDIRS={ bin="bin", lib={ "lib", [[lib/arm-linux-gnueabihf]] }, include="include" }
return site_config
