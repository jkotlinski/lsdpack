SECTION "player",ROM0[$3e70]
    ; .gbs player entry points
    ret
    jp LsdjPlaySong
    jp LsdjTick

; Starts playing a song. If a song is already playing,
; make sure interrupts are disabled when calling this.
;
; IN: a = song number
; OUT: -
; SIDE EFFECTS: trashes af
;
LsdjPlaySong::
    push    de
    push    hl

    ld  [Song],a
    ld  d,0
    add a,a
    add a,a
    ld  e,a

    ld  hl,SongLocations
    add hl,de

    xor a
    ldh [$26],a ; stop sound
    ld  [Wait],a
    ld  a,[hl+]
    ld  [CurrentBank],a
    ld  a,[hl+]
    ld  [CurrentBank+1],a
    ld  a,[hl+]
    ld  [CurrentPtr],a
    ld  a,[hl]
    ld  [CurrentPtr+1],a

    pop hl
    pop de
    ret

; Call this six times per screen update,
; evenly spread out over the screen.
;
; IN: -
; OUT: -
; SIDE EFFECTS: changes ROM bank, trashes af
;
LsdjTick::
    push    hl

    ld  hl,Wait
    ld  a,[hl]
    or  a
    jr  z,.not_waiting
    dec [hl]
    pop hl
    ret

.not_waiting
    push    de
    push    bc

.tick
    xor a
    ld  c,a ; c = exit loop flag
    ld  a,[CurrentBank+1]
    ld  [$3000],a
    ld  a,[CurrentBank]
    ld  [$2000],a
    ld  a,[CurrentPtr+1] ; hl = ptr
    ld  h,a
    ld  a,[CurrentPtr]
    ld  l,a

    ld  d,$ff

.loop
    ld  a,c
    or  a
    jr  nz,.lyc_done

    ld  a,[hl]
    and $80
    ld  c,a

    ld  a,[hl+]
    and $7f
    jr  z,.lyc_done
    cp  1
    jr  z,.handle_sample
    cp  4
    jp  z,.volume_down_pu0
    cp  5
    jp  z,.volume_down_pu1
    cp  6
    jp  z,.volume_down_noi
    cp  7
    jp  z,.pitch_pu0
    cp  8
    jp  z,.pitch_pu1
    cp  9
    jp  z,.pitch_wav
    cp  10
    jr  z,.wait
    cp  3
    jp  z,.next_bank
    cp  2
    jr  z,.handle_stop

    ; write sound register
    ld  b,a
    and $7f
    ld  e,a
    ld  a,[hl+]
    ld  [de],a
    ld  a,b
    bit 7,a ; test LYC_END
    jr  z,.loop

.lyc_done
    ld  a,l
    ld  [CurrentPtr],a
    ld  a,h
    ld  [CurrentPtr+1],a
    pop bc
    pop de
    pop hl
    ret

.wait:
    ld  a,[hl+]
    ld  [Wait],a
    jr  .lyc_done

.handle_stop
    ld  a,[Song]
    call    LsdjPlaySong
    jr  .tick

.handle_sample
    ld  a,[hl+]
    ld  b,a     ; b = sample bank
    ld  a,[hl+]
    ld  e,a
    ld  a,[hl+]
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

    ld  a,[CurrentBank+1]
    ld  [$3000],a
    ld  a,[CurrentBank]
    ld  [$2000],a

    ld  a,[hl+]
    ldh [$1e],a
    ld  a,[hl+]
    ldh [$1d],a

    ld  a,e
    ldh [$25],a

    ld  d,$ff
    jp  .loop

.next_bank
    ld  a,$40
    ld  h,a
    xor a
    ld  l,a

    ld  a,[CurrentBank]
    inc a
    ld  [CurrentBank],a
    ld  [$2000],a
    jp  nz,.loop
    ld  a,[CurrentBank+1]
    inc a
    ld  [CurrentBank+1],a
    ld  [$3000],a
    ; Reapply LSB in case of non-MBC5 cartridge.
    ld  [CurrentBank],a
    ld  [$2000],a
    jp  .loop

.volume_down_pu0:
    ld  a,9
    ldh [$12],a
    ld  a,$11
    ldh [$12],a
    ld  a,$18
    ldh [$12],a
    jp  .loop

.volume_down_pu1:
    ld  a,9
    ldh [$17],a
    ld  a,$11
    ldh [$17],a
    ld  a,$18
    ldh [$17],a
    jp  .loop

.volume_down_noi:
    ld  a,9
    ldh [$21],a
    ld  a,$11
    ldh [$21],a
    ld  a,$18
    ldh [$21],a
    jp  .loop

.pitch_pu0:
    ld  a,[hl+]
    ldh [$13],a
    ld  a,[hl+]
    ldh [$14],a
    jp  .loop

.pitch_pu1:
    ld  a,[hl+]
    ldh [$18],a
    ld  a,[hl+]
    ldh [$19],a
    jp  .loop

.pitch_wav:
    ld  a,[hl+]
    ldh [$1d],a
    ld  a,[hl+]
    ldh [$1e],a
    jp  .loop

SECTION "player_ram",WRAM0

Song
    ds  1
CurrentBank
    ds  2
CurrentPtr
    ds  2
Wait
    ds  1
