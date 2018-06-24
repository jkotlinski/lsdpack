# lsdpack

Records LSDj songs for use in stand-alone Game Boy ROMs. (e.g. your own games, demos, ...)

## Building

Requires CMake and a C++ compiler. E.g.

    cmake .
    make

## Usage

All songs in the .sav must first be prepared so that they are stopped with HFF command. If they are not, lsdpack will run indefinitely. Then, place your .sav and .gb file in the same directory and run e.g. `./lsdjpack.exe lsdj.gb` to record the songs to the assembly file `music.s`. The Game Boy player ROM can now be built using RGBDS:

    rgbasm -o music.o music.s
    rgbasm -o player.o player.s
    rgblink -o player.gb player.o music.o
    rgbfix -v -m 0x19 -p 0 player.gb
