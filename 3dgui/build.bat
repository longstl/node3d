setlocal
set path=%path%;
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"
devenv 3DGUI.sln /build release
endlocal