SECTION "player_ram",WRAM0

Song
    ds  1
CurrentBank
    ds  2
CurrentPtr
    ds  2

SECTION "player_code",ROM0

; Starts playing a song. If a song is already playing,
; make sure interrupts are disabled when calling this.
;
; IN: a = song number
; OUT: -
; SIDE EFFECTS: changes ROM bank, trashes de and hl
;
LsdjPlaySong::
    ld  [Song],a
    ld  d,0
    add a,a
    add a,a
    ld  e,a

    ld  hl,SongLocations
    add hl,de

    ld  a,[hl+]
    ld  [CurrentBank],a
    ld  [$2000],a
    ld  a,[hl+]
    ld  [CurrentBank+1],a
    ld  [$3000],a
    ld  a,[hl+]
    ld  [CurrentPtr],a
    ld  a,[hl]
    ld  [CurrentPtr+1],a
    ret

GetByte: MACRO
    ld  a,h
    cp  $80
    call    z,.next_bank
    ld  a,[hl+]
    ENDM

; Call this six times per screen update,
; evenly spread out over the screen.
;
; IN: -
; OUT: -
; SIDE EFFECTS: changes ROM bank
;
LsdjTick::
    push    af
    push    de
    push    hl

.tick
    ld  a,[CurrentPtr+1] ; hl = ptr
    ld  h,a
    ld  a,[CurrentPtr]
    ld  l,a

    ld  d,$ff

.loop
    GetByte

    or  a
    jr  z,.done
    cp  $ff
    jr  z,.handle_stop
    cp  $fe
    jr  nz,.write_byte_to_papu
    call    .handle_sample
    jr  .loop

.next_bank
    ld  a,$40
    ld  h,a

    ld  a,[CurrentBank]
    inc a
    ld  [CurrentBank],a
    ld  [$2000],a
    ret nz
    ld  a,[CurrentBank+1]
    inc a
    ld  [CurrentBank+1],a
    ld  [$3000],a
    ret

.write_byte_to_papu
    ld  e,a
    GetByte
    ld  [de],a

    jr  .loop

.handle_stop
    ld  a,[Song]
    call    LsdjPlaySong
    jr  .tick

.done
    ld  a,l
    ld  [CurrentPtr],a
    ld  a,h
    ld  [CurrentPtr+1],a

    pop hl
    pop de
    pop af
    ret

.handle_sample
    push    bc
    push    de

    GetByte
    ld  b,a     ; b = sample bank
    GetByte
    ld  e,a
    GetByte
    ld  d,a     ; de = sample ptr

    push    hl
    ; switch bank
    xor a
    ld  [$3000],a
    ld  a,b
    ld  [$2000],a

    ld  h,d
    ld  l,e

    ; disable wave channel output
    ldh a,[$25]
    ld  e,a
    and a,~$44
    ldh [$25],a

    xor a
    ldh [$1a],a ; mute channel

    ld  a,[hl+]
    ldh [$30],a
    ld  a,[hl+]
    ldh [$31],a
    ld  a,[hl+]
    ldh [$32],a
    ld  a,[hl+]
    ldh [$33],a
    ld  a,[hl+]
    ldh [$34],a
    ld  a,[hl+]
    ldh [$35],a
    ld  a,[hl+]
    ldh [$36],a
    ld  a,[hl+]
    ldh [$37],a
    ld  a,[hl+]
    ldh [$38],a
    ld  a,[hl+]
    ldh [$39],a
    ld  a,[hl+]
    ldh [$3a],a
    ld  a,[hl+]
    ldh [$3b],a
    ld  a,[hl+]
    ldh [$3c],a
    ld  a,[hl+]
    ldh [$3d],a
    ld  a,[hl+]
    ldh [$3e],a
    ld  a,[hl+]
    ldh [$3f],a

    ld  a,$80 ; unmute
    ldh [$1a],a

    pop     hl

    ld  a,[CurrentBank]
    ld  [$2000],a
    ld  a,[CurrentBank+1]
    ld  [$3000],a

    GetByte
    ldh [$1e],a
    GetByte
    ldh [$1d],a

    ld  a,e
    ldh [$25],a

    pop de
    pop bc
    ret
