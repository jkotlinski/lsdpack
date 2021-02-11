rm -rf bin
mkdir bin
pushd bin

/c/Program\ Files/CMake/bin/cmake.exe ..
/c/Program\ Files/CMake/bin/cmake.exe --build . --config Release --target PACKAGE

popd
