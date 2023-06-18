$prefix = "[{0}/{1}]`t"

$cc = "gcc"
$cxx = "g++"
$comptool = ".\dist\comptool.exe"

$common_rel_flags = @('-Isrc', '-pipe', '-static', '-m64', '-Os', '-flto', '-fPIC', '-fPIE', '-ffunction-sections', '-fdata-sections', '-Wl,-s,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none', '-msse', '-mavx', '-mmmx', '-DNDEBUG', '-funsafe-math-optimizations', '-ftree-vectorize', '-ffast-math', '-ftree-slp-vectorize', '-fassociative-math', '-fvisibility=hidden', '-fcompare-debug-second', '-fno-exceptions', '-fno-stack-protector', '-fno-math-errno', '-fno-ident', '-fno-asynchronous-unwind-tables')

$c_release_flags = ($common_rel_flags + @('-std=gnu99', '-nostartfiles', '-nodefaultlibs', '-nostdlib', '-nolibc'))
$cpp_release_flags = ($common_rel_flags + @('-fpermissive', '-w', '-std=gnu++23', '-nostartfiles'))

function CC($in,$out,$flags) {
	PrintStatus "CC `t$out"
	Exec "$cc -o $out $in @flags"
}

function CXX($in,$out,$flags) {
	PrintStatus "CXX `t$out"
	Exec "$cxx -o $out $in @flags"
}

function Compress($in,$out) {
	PrintStatus "COMP `t$out"
	Exec "$comptool 2 $in $out"
}

Build @{
	'dist\inject.exe'     = @('CC',  @('src/hook.c'), (@('-lshlwapi', '-mwindows', '-Wl,-e,WinMain') + $c_release_flags));
	'dist\comptool.exe'   = @('CC',  @('tools/comptool.c'), @('-luser32'));

	'bin\getinput.dll'    = @('CC',  @('src/getinput.c'), (@('-shared', '-Wl,-e,DllMain', '-luser32', '-lkernel32', '-lntdll') + $c_release_flags));
	'bin\discordrpc.dll'  = @('CXX', @('src/discord.cpp', 'src/extern/discord-rpc/src/connection_win.cpp', 'src/extern/discord-rpc/src/discord_register_win.cpp', 'src/extern/discord-rpc/src/discord_rpc.cpp', 'src/extern/discord-rpc/src/rpc_connection.cpp', 'src/extern/discord-rpc/src/serialization.cpp'), (@('-shared', '-Isrc/extern/discord-rpc/include', '-Isrc/extern/rapidjson/include', '-DDISCORD_WINDOWS', '-DDISCORD_DISABLE_IO_THREAD', '-DDISCORD_DYNAMIC_LIB', '-DDISCORD_BUILDING_SDK', '-Wl,-e,DllMain') + $cpp_release_flags));
	'bin\map_rndr.dll'    = @('CXX', @('src/map_renderer.cpp'), (@('-shared', '-Wl,-e,DllMain') + $cpp_release_flags));

	'dist\getinput.dll'   = @('Compress', @('bin\getinput.dll'));
	'dist\map_rndr.dll'   = @('Compress', @('bin\map_rndr.dll'));
	'dist\discordrpc.dll' = @('Compress', @('bin\discordrpc.dll'));

	'build'               = @('', @('dist\comptool.exe', 'dist\inject.exe', 'dist\getinput.dll', 'dist\map_rndr.dll', 'bin\discordrpc.dll'));
}
