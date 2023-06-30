echo %cd%
copy ..\x64\release\*.dll ..\_dist
if %errorlevel% neq 0 pause