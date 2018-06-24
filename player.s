PlayerScope

SECTION "player_ram",WRAM0

.bank
    ds  2
.ptr
    ds  2
.song
    ds  1

SECTION "boot",ROM0[$100]
    jr  $150

SECTION "player_code",ROM0[$150]
    ; .bank = 1
    ; .ptr = $4000
    xor a
    ld  [.song],a
    ld  [.bank+1],a
    ld  [$3000],a
    ld  [.ptr],a
    inc a
    ld  [.bank],a
    ld  [$2000],a
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

    ld  a,$10
    ldh [0],a

.wait_button_pressed
    ldh a,[0]
    and $f
    cp  $f
    jr  z,.wait_button_pressed

    di
    xor a
    ldh [$26],a ; mute
    call    .next_song
    ei

    call    .delay

.wait_button_released
    ldh a,[0]
    and $f
    cp  $f
    jr  nz,.wait_button_released

    call    .delay

    jr  .wait_button_pressed

.delay
    xor a
.wait
    inc a
    jr  nz,.wait
    ret

; -------------

.next_bank
    ld  a,$40
    ld  h,a

    ld  a,[.bank]
    inc a
    ld  [.bank],a
    ld  [$2000],a
    ret nz
    ld  a,[.bank+1]
    inc a
    ld  [.bank+1],a
    ld  [$3000],a
    ret

.on_lcd_interrupt
    push    af
    push    de
    push    hl

    ld  a,[.ptr+1] ; hl = ptr
    ld  h,a
    ld  a,[.ptr]
    ld  l,a

    ld  d,$ff

.loop
    ld  a,h
    cp  $80
    call    z,.next_bank

    ld  a,[hl+]
    or  a
    jr  z,.lyc_done
    cp  $ff
    jr  z,.handle_stop

    ld  e,a

    ld  a,h
    cp  $80
    call    z,.next_bank

    ld  a,[hl+]
    ld  [de],a

    jr  .loop

.handle_stop
    call    .next_song
    jr  .prepare_next_lyc

.next_song
    ld  hl,.song
    inc [hl]
    ld  a,[hl]
    add a,a
    ld  d,0
    ld  e,a

    ld  hl,SongPtr
    add hl,de
    ld  a,[hl+]
    ld  [.ptr],a
    ld  a,[hl]
    ld  [.ptr+1],a
    or  a
    jr  z,.reached_last_song

    ld  hl,SongBank
    add hl,de
    ld  a,[hl+]
    ld  [.bank],a
    ld  [$2000],a
    ld  a,[hl]
    ld  [.bank+1],a
    ld  [$3000],a
    ret

.reached_last_song
    ld  a,$ff
    ld  [.song],a
    jr  .next_song

.lyc_done
    ld  a,l
    ld  [.ptr],a
    ld  a,h
    ld  [.ptr+1],a

.prepare_next_lyc
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

SECTION "lcd_handler",ROM0[$48]
    jp  .on_lcd_interrupt
