@echo off
if not exist build mkdir build
pushd build

:: copy ..\libs\SDL2.dll .\

set release_flags= -MT -nologo -Zi -O2 -FC
set debug_flags= -MD -nologo -Zi -Od -FC
set common_flags= %debug_flags%
set includes= -I..\ -I..\third_party\sdl\

:: karena kita hanya butuh mengcompile dear imgui sekali (library tidak berubah),
:: kita bisa melewati kompilasi imgui*.cpp,
:: lalu link imgui*.obj agar kompilasi cepat
:: uncomment baris dibawah jika ragu (kompilasi akan lambat!)
:: cl %common_flags% -c %includes% ../third_party/dear_imgui/imgui*.cpp

:: /subsystem:windows /entry:mainCRTStartup 
cl %common_flags% %includes% ..\main.cpp /link /out:filex.exe imgui*.obj ..\libs\SDL2.lib ..\libs\SDL2main.lib opengl32.lib shell32.lib Ole32.lib comdlg32.lib Shlwapi.lib

popd
