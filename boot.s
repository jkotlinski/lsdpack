; lsdpack - standalone LSDj (Little Sound Dj) recorder + player {{{
; Copyright (C) 2018  Johan Kotlinski
; https://www.littlesounddj.com
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along
; with this program; if not, write to the Free Software Foundation, Inc.,
; 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. }}}


; An example for how to call the player. Displays CPU usage
; using raster bars. Press A to skip to the next song.
;
; Replace with your own game, music selector or whatever
; you feel like :)

SECTION "lcd_interrupt",ROM0[$48]
    jp  lcd_interrupt_handler

SECTION "boot",ROM0[$100]
    jr  $150

SECTION "setup",ROM0[$150]
    ; setup joypad reading
    ld  a,$10
    ldh [0],a

    ; setup lyc interrupt
    ldh a,[$41]
    or  a,$40
    ldh [$41],a ; stat
    ld  a,20
    ldh [$45],a ; lyc
    ld  a,2
    ldh [$ff],a ; ie

    ; play song 0
    xor a
    ld  e,a ; e = song
    call LsdjPlaySong
    ei

mainloop

.wait_button_pressed
    ldh a,[0]
    and $f
    cp  $f
    jr  z,.wait_button_pressed

    ; play next song
    inc e
    ld  a,e
    ; reached last song?
    cp  a,(SongLocationsEnd - SongLocations) / 4
    jr  nz,.play_song
    xor a ; go back to first song
    ld  e,a
.play_song
    di
    call    LsdjPlaySong
    ei

    call    .delay

.wait_button_released
    ldh a,[0]
    and $f
    cp  $f
    jr  nz,.wait_button_released

    call    .delay

    jr  mainloop

.delay
    xor a
.delay_loop
    inc a
    jr  nz,.delay_loop
    ret

lcd_interrupt_handler
    push    af

    ld  a,$ff   ; black background
    ldh [$47],a

    call LsdjTick

    ldh a,[$45]
    cp  a,10
    jr  z,.lcd10
    cp  a,36
    jr  z,.lcd36
    cp  a,61
    jr  z,.lcd61
    cp  a,87
    jr  z,.lcd87
    cp  a,113
    jr  z,.lcd113
    ld  a,10
.write_lyc
    ldh [$45],a

    xor a   ; white background
    ldh [$47],a

    pop af
    reti

.lcd10
    ld  a,36
    jr  .write_lyc
.lcd36
    ld  a,61
    jr  .write_lyc
.lcd61
    ld  a,87
    jr  .write_lyc
.lcd87
    ld  a,113
    jr  .write_lyc
.lcd113
    ld  a,138
    jr  .write_lyc
