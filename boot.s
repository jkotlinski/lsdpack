; This is an example for how to call the player.
; Replace with your own game, music selector,
; or whatever you feel like :)

Boot

SECTION "lcd_interrupt",ROM0[$48]
    jp  .lcd_interrupt_handler

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

.mainloop

.wait_button_pressed
    ldh a,[0]
    and $f
    cp  $f
    jr  z,.wait_button_pressed

    ; play next song
    inc e
    ld  a,e
    di
    call    LsdjPlaySong
    ei

.wait_button_released
    ldh a,[0]
    and $f
    cp  $f
    jr  nz,.wait_button_released

    jr  .mainloop

.lcd_interrupt_handler
    push    af

    ld  a,$ff   ; black background
    ldh [$47],a

    push    bc
    push    de
    push    hl

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

    pop hl
    pop de
    pop bc

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
