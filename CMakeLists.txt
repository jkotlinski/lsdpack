cmake_minimum_required (VERSION 3.1)
project (lsdpack)

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O2")
endif()

add_definitions(-DHAVE_STDINT_H)

include_directories(
    libgambatte/common
    libgambatte/include
    libgambatte/src
    libgambatte/src/mem)

add_executable(lsdpack

    lsdpack.cpp
    writer.cpp
    getopt.cpp

    libgambatte/src/bitmap_font.cpp
    libgambatte/src/cpu.cpp
    libgambatte/src/gambatte.cpp
    libgambatte/src/initstate.cpp
    libgambatte/src/interrupter.cpp
    libgambatte/src/interruptrequester.cpp
    libgambatte/src/memory.cpp
    libgambatte/src/sound.cpp
    libgambatte/src/state_osd_elements.cpp
    libgambatte/src/statesaver.cpp
    libgambatte/src/tima.cpp
    libgambatte/src/video.cpp
    libgambatte/src/file/file.cpp
    libgambatte/src/mem/cartridge.cpp
    libgambatte/src/mem/memptrs.cpp
    libgambatte/src/mem/pakinfo.cpp
    libgambatte/src/mem/rtc.cpp
    libgambatte/src/sound/channel1.cpp
    libgambatte/src/sound/channel2.cpp
    libgambatte/src/sound/channel3.cpp
    libgambatte/src/sound/channel4.cpp
    libgambatte/src/sound/duty_unit.cpp
    libgambatte/src/sound/envelope_unit.cpp
    libgambatte/src/sound/length_counter.cpp
    libgambatte/src/video/ly_counter.cpp
    libgambatte/src/video/lyc_irq.cpp
    libgambatte/src/video/next_m0_time.cpp
    libgambatte/src/video/ppu.cpp
    libgambatte/src/video/sprite_mapper.cpp)

add_executable(makegbs makegbs.cpp getopt.cpp)

set_property(TARGET lsdpack PROPERTY CXX_STANDARD 98)
