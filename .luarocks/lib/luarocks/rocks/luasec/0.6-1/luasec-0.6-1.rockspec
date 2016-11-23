package = "LuaSec"
version = "0.6-1"
source = {
   url = "git://github.com/brunoos/luasec.git",
   tag = "luasec-0.6"
}
description = {
   summary = "A binding for OpenSSL library to provide TLS/SSL communication over LuaSocket.",
   detailed = "This version delegates to LuaSocket the TCP connection establishment between the client and server. Then LuaSec uses this connection to start a secure TLS/SSL session.",
   homepage = "https://github.com/brunoos/luasec/wiki",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1", "luasocket"
}
external_dependencies = {
   platforms = {
      unix = {
         OPENSSL = {
            header = "openssl/ssl.h",
            library = "ssl"
         }
      },
      windows = {
         OPENSSL = {
            header = "openssl/ssl.h",
         }
      },
   }
}
build = {
   type = "builtin",
   copy_directories = {
      "samples"
   },
   platforms = {
      unix = {
         install = {
            lib = {
               "ssl.so"
            },
            lua = {
               "src/ssl.lua", ['ssl.https'] = "src/https.lua"
            }
         },
         modules = {
            ssl = {
               incdirs = {
                  "$(OPENSSL_INCDIR)", "src/", "src/luasocket",
               },
               libdirs = {
                  "$(OPENSSL_LIBDIR)"
               },
               libraries = {
                  "ssl", "crypto"
               },
               sources = {
                  "src/x509.c", "src/context.c", "src/ssl.c", 
                  "src/luasocket/buffer.c", "src/luasocket/io.c",
                  "src/luasocket/timeout.c", "src/luasocket/usocket.c"
               }
            }
         }
      },
      windows = {
         install = {
            lib = {
               "ssl.dll"
            },
            lua = {
               "src/ssl.lua", ['ssl.https'] = "src/https.lua"
            }
         },
         modules = {
            ssl = {
               defines = {
                  "WIN32", "NDEBUG", "_WINDOWS", "_USRDLL", "LSEC_EXPORTS", "BUFFER_DEBUG", "LSEC_API=__declspec(dllexport)",
                  "LUASEC_INET_NTOP", "WINVER=0x0501", "_WIN32_WINNT=0x0501", "NTDDI_VERSION=0x05010300"
               },
               libdirs = {
                  "$(OPENSSL_LIBDIR)",
                  "$(OPENSSL_BINDIR)",
               },
               libraries = {
                  "libeay32", "ssleay32", "ws2_32"
               },
               incdirs = {
                  "$(OPENSSL_INCDIR)", "src/", "src/luasocket"
               },
               sources = {
                  "src/x509.c", "src/context.c", "src/ssl.c", 
                  "src/luasocket/buffer.c", "src/luasocket/io.c",
                  "src/luasocket/timeout.c", "src/luasocket/wsocket.c"
               }
            }
         },
         patches = {
            ["luarocks_vs_compiler.patch"] = [[
--- a/src/ssl.c.orig
+++ b/src/ssl.c
@@ -844,3 +844,8 @@ LSEC_API int luaopen_ssl_core(lua_State *L)

   return 1;
 }
+
+#if defined(_MSC_VER)
+/* Empty implementation to allow building with LuaRocks and MS compilers */
+LSEC_API int luaopen_ssl(lua_State *L) { return 0; }
+#endif
]]
         }
      }
   }
}
