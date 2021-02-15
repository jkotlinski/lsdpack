cmake_minimum_required (VERSION 3.1)
project (lsdpack)

set(CPACK_PACKAGE_VENDOR "Johan Kotlinski")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 4)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
if(MSVC)
    set(CPACK_GENERATOR "ZIP")
endif()
install(FILES
    ${CPACK_RESOURCE_FILE_LICENSE}
    ${CPACK_RESOURCE_FILE_README}
    DESTINATION .)
include (CPack)

add_definitions(-DHAVE_STDINT_H)

include_directories(
    libgambatte/common
    libgambatte/include
    libgambatte/src
    libgambatte/src/mem)

add_executable(lsdpack

    lsdpack.cpp
    getopt.cpp
    writer.cpp
    rules/rule_envelope.cpp
    rules/rule_pitch.cpp
    rules/rule_sample.cpp
    rules/rule_interrupted_sample.cpp
    rules/rule_repeat_command.cpp

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

get_directory_property(TARGET_NAMES BUILDSYSTEM_TARGETS)

install(TARGETS ${TARGET_NAMES})

foreach(TARGET_NAME ${TARGET_NAMES})
    set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 14)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic)
    endif()
endforeach()
