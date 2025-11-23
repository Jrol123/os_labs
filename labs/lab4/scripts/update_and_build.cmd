@echo off
echo Pulling latest changes from Git...
git pull

echo Building the project...
mkdir build 2>nul
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

echo Build complete! Executable is in the .\build directory
pause