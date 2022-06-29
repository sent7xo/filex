@echo off
if not exist build mkdir build
pushd build

:: copy ..\libs\SDL2.dll .\

set common_flags= -nologo -Zi -FC
set release_flags= -MT -O2
set debug_flags= -MD -Od
set compiler_flags= %common_flags% %debug_flags%
set includes= -I..\ -I..\third_party\sdl\
set libraries= ..\libs\SDL2.lib ..\libs\SDL2main.lib opengl32.lib shell32.lib ole32.lib comdlg32.lib shlwapi.lib

:: karena kita hanya butuh mengcompile dear imgui sekali (library tidak berubah),
:: kita bisa melewati kompilasi imgui*.cpp,
:: lalu link imgui*.obj agar kompilasi cepat
:: uncomment baris dibawah jika ragu (kompilasi akan lambat!)
:: cl %compiler_flags% -c %includes% ../third_party/dear_imgui/imgui*.cpp

:: /subsystem:windows /entry:mainCRTStartup 
cl %compiler_flags% %includes% ..\main.cpp imgui*.obj /link /out:filex.exe %libraries%

popd

copy build\filex.exe .
