package = "LuaCrypto"
version = "0.3.2-2"
description = {
	summary = "A Lua frontend to OpenSSL",
	detailed = [[LuaCrypto is a Lua frontend to the OpenSSL cryptographic library. The OpenSSL features that are currently exposed are: 
digests (MD5, SHA-1, HMAC, and more), encryption, decryption and crypto-grade random number generators.]],
	homepage = "https://github.com/starius/luacrypto",
	license = "MIT",
}
dependencies = {
	"lua >= 5.1",
}
external_dependencies = {
	OPENSSL = {
		header = "openssl/evp.h"
	}
}
source = {
	url = "git://github.com/starius/luacrypto.git",
}
build = {
	type = "builtin",
	modules = {
		crypto = {
			sources = "src/lcrypto.c",
			incdirs = "$(OPENSSL_INCDIR)",
			libdirs = "$(OPENSSL_LIBDIR)",
			libraries = "crypto",
		}
	},
	platforms = {
		windows = {
			modules = { crypto = { libraries = {'libeay32', 'ssleay32', 'kernel32', 'user32', 'gdi32', 'advapi32'} } }
		}
	},
	copy_directories = { "doc" }
}
build.platforms.mingw32 = build.platforms.windows
