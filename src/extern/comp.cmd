@echo off
:: del *.o *.a

set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
g++ -Os -fvisibility=hidden -fcompare-debug-second -flto -fno-rtti -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -c %sources: = discord-rpc/src/% -Idiscord-rpc/src -Idiscord-rpc/include -Irapidjson/include -fPIC -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK -DNDEBUG

::cl /Os /c /DDISCORD_WINDOWS /DDISCORD_DISABLE_IO_THREAD /DDISCORD_DYNAMIC_LIB /DDISCORD_BUILDING_SDK /DNDEBUG /Idiscord-rpc/src /Idiscord-rpc/include /Irapidjson/include /I.\ %sources: = discord-rpc/src/%