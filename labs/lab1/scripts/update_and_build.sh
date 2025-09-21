#!/bin/bash
echo "Pulling latest changes from Git..."
git pull

echo "Building the project..."
# Если используется out-of-source build (лучшая практика)
mkdir -p build
cd build
cmake ..
make

echo "Build complete! Executable is in the ./build directory"