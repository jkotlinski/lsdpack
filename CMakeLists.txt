cmake_minimum_required (VERSION 2.8.4)
project (lsdpack)
add_executable(lsdpack
    lsdpack.cpp
    rom.cpp
    sav.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
