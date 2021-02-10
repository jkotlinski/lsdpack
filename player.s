SECTION "player",ROM0[$3d00]
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

    ld  hl,SongLocations
    add a,a
    add a,a
    add a,l
    ld  l,a

    ld  de,CurrentBank
    ld  a,[hl+]
    ld  [de],a  ; CurrentBank
    inc de
    ld  a,[hl+]
    ld  [de],a  ; CurrentBank + 1
    inc de
    ld  a,[hl+]
    ld  [de],a  ; CurrentPtr
    inc de
    ld  a,[hl+]
    ld  [de],a  ; CurrentPtr + 1

    xor a
    ldh [$26],a ; stop sound
    ld  [RepeatCmdCounter],a

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
    push    de
    push    bc

.tick
    ld  a,[CurrentBank+1]
    ld  [$3000],a
    ld  a,[CurrentBank]
    ld  [$2000],a
    ld  a,[CurrentPtr+1] ; hl = ptr
    ld  h,a
    ld  a,[CurrentPtr]
    ld  l,a

    xor a
    ld  b,a ; b = last read command byte

.loop
    bit 7,b
    jr  nz,.lyc_done

    ; load repeated cmd
    ld  a,[RepeatCmdCounter]
    or  a
    jr  z,.load_new_cmd_from_stream
    dec a
    ld  [RepeatCmdCounter],a
    ld  a,[RepeatCmd]
    jr  .apply_cmd

.load_new_cmd_from_stream
    ld  a,[hl+]
    bit 6,a
    jr  z,.apply_cmd

    ; save repeated cmd
    and ~$40
    ld  [RepeatCmd],a
    ld  b,a
    ld  a,[hl+]
    ld  [RepeatCmdCounter],a
    ld  a,b

.apply_cmd
    ld  b,a
    and $7f

    jr  z,.lyc_done
    cp  1
    jp  z,.sample_start
    cp  10
    jp  z,.sample_next
    cp  4
    jr  z,.volume_down_pu0
    cp  5
    jr  z,.volume_down_pu1
    cp  6
    jr  z,.volume_down_noi
    cp  7
    jr  z,.pitch_pu0
    cp  8
    jr  z,.pitch_pu1
    cp  9
    jr  z,.pitch_wav
    cp  3
    jr  z,.next_bank
    cp  2
    jr  z,.handle_stop

    ; write sound register
    ld  c,a
    ld  a,[hl+]
    ldh [c],a
    jr  .loop

.lyc_done
    ld  a,l
    ld  [CurrentPtr],a
    ld  a,h
    ld  [CurrentPtr+1],a
    pop bc
    pop de
    pop hl
    ret

.handle_stop:
    ld  a,[Song]
    call    LsdjPlaySong
    jp  .tick

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

.next_bank:
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

.sample_start:
    ld  a,[hl+]
    ld  c,a     ; c = sample bank
    ld  [SampleBank],a
    ld  a,[hl+]
    ld  e,a
    ld  a,[hl+]
    ld  d,a     ; de = sample ptr

    push    hl
    ; switch bank
    xor a
    ld  [$3000],a
    ld  a,c
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

    ld  a,l
    ld  [SampleAddress],a
    ld  a,h
    ld  [SampleAddress+1],a

    ld  a,$80 ; unmute
    ldh [$1a],a

    pop     hl

    ld  a,[CurrentBank+1]
    ld  [$3000],a
    ld  a,[CurrentBank]
    ld  [$2000],a

    ld  a,[hl+]
    ldh [$1e],a
    ld  [SamplePitchMsb],a
    ld  a,[hl+]
    ldh [$1d],a

    ld  a,e
    ldh [$25],a

    jp  .loop

.sample_next:
    ld  a,[SampleBank]
    ld  c,a     ; c = sample bank
    ld  a,[SampleAddress]
    ld  e,a
    ld  a,[SampleAddress+1]
    ld  d,a     ; de = sample ptr

    push    hl
    ; switch bank
    xor a
    ld  [$3000],a
    ld  a,c
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

    ld  a,l
    ld  [SampleAddress],a
    ld  a,h
    ld  [SampleAddress+1],a

    ld  a,$80 ; unmute
    ldh [$1a],a

    pop     hl

    ld  a,[CurrentBank+1]
    ld  [$3000],a
    ld  a,[CurrentBank]
    ld  [$2000],a

    ld  a,[SamplePitchMsb]
    ldh [$1e],a

    ld  a,e
    ldh [$25],a

    jp  .loop


SECTION "player_ram",WRAM0

Song
    ds  1
CurrentBank
    ds  2
CurrentPtr
    ds  2
SampleBank
    ds  1
SampleAddress
    ds  2
SamplePitchMsb
    ds  1
RepeatCmd
    ds  1
RepeatCmdCounter
    ds  1
