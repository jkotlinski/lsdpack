# lsdpack

Records LSDj songs to an RGBDS compatible player that can be used in stand-alone ROMs. (e.g. your own games, demos, ...)

## Building

Requires CMake and a compatible C++ compiler. E.g.

    cmake .
    make

## Usage

All songs in the .sav must first be prepared so that they are stopped with HFF command. If they are not, lsdpack will run indefinitely.

Then, place your .sav and .gb file in the same directory and run e.g.

    ./lsdjpack.exe lsdj.gb
    
This will produce the file music.s, which contains the recorded sound data. The Gameboy player ROM can now be built by:

    rgbasm -o music.o music.s
    rgbasm -o player.o player.s
    rgblink -o player.gb player.o music.o
    rgbfix -v -m 0x19 -p 0 player.gb
