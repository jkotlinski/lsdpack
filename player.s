PlayerScope

SECTION "player_ram",WRAM0

.bank
    ds  2
.ptr
    ds  2

SECTION "boot",ROM0[$100]
    jr  $150

SECTION "player_code",ROM0[$150]
    ; initialize bank + ptr
    ld  hl,$3000
    xor a
    ld  [.bank+1],a
    ld  [hl],a
    ld  [.ptr],a
    ld  h,$20
    inc a
    ld  [.bank],a
    ld  [hl],a
    ld  a,$40
    ld  [.ptr+1],a

    ; setup lyc interrupt
    ldh a,[$41]
    or  a,$40
    ldh [$41],a ; stat
    ld  a,20
    ldh [$45],a ; lyc
    ld  a,2
    ldh [$ff],a ; ie
    ei

.mainloop
    jr  .mainloop

.on_lcd_interrupt
    ; prepare new lyc
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

SECTION "lcd_handler",ROM0[$48]
    jp  .on_lcd_interrupt
