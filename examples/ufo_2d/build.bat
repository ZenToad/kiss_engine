@echo off
if not exist bin mkdir bin
pushd bin 
set INCLUDE_FILES=-I../../../include -I../../../lib/Box2D/ -I../../../lib/glfw/include
set LIBRARY_FILES=Gdi32.lib opengl32.lib Shell32.lib user32.lib ../../../lib/glfw/build/src/Debug/glfw3.lib ../../../lib/Box2D/Build/vs2017/bin/Release/Box2D.lib
set COMMON_FILES= ../../../include/glad/glad.c ../../../include/imgui/imgui.cpp ../../../include/imgui/imgui_draw.cpp ../../../include/imgui/imgui_demo.cpp 
REM /FC Full source code paths /Zi Debug info /WX warnings as errors /W4 warning level 4 
REM /Oi intrinsic functions /GR- disable RTTI /EHa- disable exceptions /MD use dlls, should be /MT but not working
set FLAGS=-nologo -D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES /Oi /GR- /Gm /EHa- /WX /W4 /FC /Zi /MD /wd4100 /wd4456 /wd4244 /wd4201 /wd4305 /wd4459 /wd4577 /wd4189

cl %FLAGS% %INCLUDE_FILES% ../ufo_2d.cpp %COMMON_FILES% /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:MSVCRTD %LIBRARY_FILES%
xcopy ufo_2d.exe .. /i /y
popd

