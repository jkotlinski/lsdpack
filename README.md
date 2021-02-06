# lsdpack

Records LSDj songs for use in stand-alone Game Boy ROMs. (E.g. your own games, demos, music albums...)

## Building
Requires CMake and a C++ compiler. Exact build steps are platform dependent - see [Running CMake](https://cmake.org/runningcmake/)

![C++ CI with CMake](https://github.com/jkotlinski/lsdpack/workflows/CMake-Ubuntu/badge.svg)
![C++ CI with CMake](https://github.com/jkotlinski/lsdpack/workflows/CMake-Windows/badge.svg)

## Recording Songs

All songs in the .sav must first be prepared so that they are eventually stopped with the HFF command. Then, place your .sav and .gb file in the same directory and run e.g. `./lsdpack.exe lsdj.gb` to record the songs to `lsdj.s`. To record songs from several .gb files, add them all to the command line, like `./lsdpack.exe 1.gb 2.gb 3.gb`.

## Playing Songs from Your Own Code

An example Game Boy player ROM can be built using RGBDS:

    rgbasm -o boot.o boot.s
    rgbasm -o player.o player.s
    rgbasm -o lsdj.o lsdj.s
    rgblink -o player.gb boot.o player.o lsdj.o
    rgbfix -v -m 0x19 -p 0 player.gb

## Game Boy Sound System (GBS)

Use below commands to make a .gbs file. The makegbs -s option must be changed to the number of songs.

	./lsdpack.exe -g lsdj.gb
	rgbasm -o player_gbs.o player_gbs.s
	rgbasm -o lsdj.o lsdj.s
	rgblink -o player.gb player_gbs.o lsdj.o
	./makegbs.exe -s 1 -t "Better Off Alone" -a "Alice Deejay" -c "(C) Violent Music 1997" player.gb

### player.s

Contains the player code. Following symbols are exported:

    ; IN: a = song number
    ; OUT: -
    ; SIDE EFFECTS: trashes af
    ;
    ; Starts playing a song. If a song is already playing,
    ; make sure interrupts are disabled when calling this.
    ;
    LsdjPlaySong::

    ; IN: -
    ; OUT: -
    ; SIDE EFFECTS: changes ROM bank, trashes af
    ;
    ; Call this six times per screen update,
    ; evenly spread out over the screen.
    ;
    LsdjTick::

    ; A table that holds the ROM position for each song.
    ; Each entry is 4 bytes big.
    SongLocations::
    SongLocationsEnd::

### boot.s

An example for how to use the player. Displays CPU usage
using raster bars. Pressing A skips to the next song.

![Screenshot](/docs/screenshot.png)

## How Does It Work?

lsdpack plays back LSDj songs using an emulated Game Boy Color and records direct writes to the sound chip. This recording can be played back from another ROM using a custom player.

The included player is very fast and can easily play songs that would choke LSDj on a Game Boy Classic. Since recordings take a lot of ROM, an MBC5 cartridge is required.

ROM+CPU consumption varies with song contents. Features like sample playback or FAST/DRUM P and V commands are especially demanding, since they update the sound chip 360 times/second.
