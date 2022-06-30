@echo off
:: gunakan folder build supaya folder filex tidak ada file kompilasi, debug, dll.
if not exist build mkdir build
pushd build

:: opsional
copy ..\libs\SDL2.dll .\

set common_flags= -nologo -Zi -FC
:: for some reason, using -O2 for release makes the app unable to open change wallpaper dialog
set release_flags= -MT -Od
set debug_flags= -MD -Od

:: ganti ke release_flags untuk release, debug_flags untuk debug
set compiler_flags= %common_flags% %release_flags%

set includes= -I..\ -I..\third_party\sdl\
set libraries= ..\libs\SDL2.lib ..\libs\SDL2main.lib opengl32.lib shell32.lib ole32.lib comdlg32.lib shlwapi.lib

:: karena kita hanya butuh mengcompile dear imgui sekali (library tidak berubah),
:: kita bisa melewati kompilasi imgui*.cpp,
:: lalu link imgui*.obj agar kompilasi cepat
:: uncomment baris dibawah jika ragu (kompilasi akan lambat!)
cl %compiler_flags% -c %includes% ../third_party/dear_imgui/imgui*.cpp

cl %compiler_flags% %includes% ..\main.cpp imgui*.obj /link /out:filex.exe %libraries% /subsystem:windows /entry:mainCRTStartup

popd

:: kita membutuhkan SDL2.dll untuk menjalankan aplikasi
copy build\filex.exe .
copy libs\SDL2.dll .
