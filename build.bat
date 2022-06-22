@echo off
if not exist build mkdir build
pushd build

:: copy ..\libs\SDL2.dll .\

set release_flags= -MT -nologo -Zi -O2 -FC
set debug_flags= -MD -nologo -Zi -Od -FC
set common_flags= %debug_flags%

:: compile dear imgui secara manual,
:: lalu link imgui*.obj agar kompilasi main.cpp cepat
:: cl %common_flags% /c /I..\ ../third_party/dear_imgui/imgui*.cpp

cl %common_flags% -I..\ ..\main.cpp /link /out:filex.exe imgui*.obj ..\libs\SDL2.lib ..\libs\SDL2main.lib opengl32.lib shell32.lib Ole32.lib

popd
