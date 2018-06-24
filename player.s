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

.mainloop
    jr  .mainloop

.on_lcd_interrupt
    reti

SECTION "lcd_handler",ROM0[$48]
    jp  .on_lcd_interrupt
