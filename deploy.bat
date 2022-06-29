@echo off
if exist filex rmdir /s filex
mkdir filex

xcopy /i data filex\data
copy build\filex.exe filex
copy libs\SDL2.dll filex\
