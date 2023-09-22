@echo off
setlocal enabledelayedexpansion

mode 125,55

set /a rasterx=rastery=8
rundll32 ..\..\getinput.dll,inject

:a
title !controller1_ltrig! !controller1_rtrig! !controller1_lthumbx! !controller1_lthumby! !controller1_rthumbx! !controller1_rthumby!


rem for normal style controller input
break || (

for /l %%c in (1,1,4) do (
	for /f "tokens=1,2 delims=^|" %%a in ("!controller%%c!") do (
		if not "%%b"=="" set controller%%c_btns=%%b

		set analograw=%%a
		set /a "controller%%c_!analograw:,=,controller%%c_!"
	)
)

)

rem yeshis shit requirements


goto :a