CMP AH,84   ;80FC84      fn 84?
JNZ jmp1    ;750A jmp +0A    if not jump to exit
CMP DX,+00  ;83FA00      Dx=0? (same as AL=0, which is more proper)
JZ jmp2     ;740C        get switches
CMP DX,+01  ;83FA01      DX=1 (same as Al=1)
JZ jmp3      ;7413       get position
.jmp1      
STI         ;FB       
CMP AH,80   ;80FC80     
JMP B7C7    ;e9B1b6     exit
.jmp2                  ; do switches
CLI         ;FA         ints off
MOV DX,0201 ;BA0102
IN AL,DX    ;EC         get port 201
;NEG AL      ;F6D8       This is wrong. My BIOS book says bottom 4 bits are 
                        set to 0, so that would be AND AL,F0h. 
                        Stuart says XOR AL,ffh...which will swap the state..
                        is this required?
AND AL,F0h                        
STI         ;FB        enable ints and clear carry
CLC         ;f8
RETF 0002   ;CA0200    return
.jmp3                 ;do position
CLI         ;FA        ints off
PUSH BP     ;55        save BP
MOV AX,0000 ;b80000    clear AX and then temp data space on the stack.
PUSH AX     ;50
PUSH AX     ;50
PUSH AX     ;50
PUSH AX     ;50
MOV BP,SP   ;89e5      copy stack pointer.
MOV CX,0000 ;b90000    cx=0
MOV BX,000f ;bb0f00    bx=f
MOV DX,0201 ;ba0102
OUT DX,AL   ;ee        get port201
.jmp4
INC CX      ;41        CX++
CMP CX,0300 ;81f90003  got to 300 yet? if so return results anyway
JG  finished    ;7f2b  
IN AL,DX    ;ec        get 201
AND AL,BL   ;20d8      mask out bottom nibble of AL
CMP AL,BL   ;38d8      
JZ jmp4     ;74f2      If AL=0f go round again.
DEC CX      ;49        C--, as we have now gone one too far
XOR AL,BL   ;30D8      invert bottom 4 bits of AL
TEST AL,01     ;A801   if bit 1 set then save CX as stick A X-coord
JZ jmp5        ;7403
MOV [BP+00],CX ;894e00
.jmp5
TEST al,02     ;a802   if bit 2 set then save stick A Y-coord
JZ jmp6
MOV [BP+02],CX ;894e02
.jmp6
TEST al,04     ;a804   if bit 3 set then save stick B X-coord
JZ jmp7
MOV [BP+04],CX ;894e04
.jmp7
TEST al,08     ;a808   if bit 4 set then save stick B Y-coord
JZ jmp8
MOV [BP+06],CX ;894e06
.jmp8
INC CX         ;41     C++
XOR BL,AL      ;30C3
JNZ jmp4       ;75CE     do next stick
.finished
CLC            ;f8   clear carry
POP AX         ;58   stick A, X-coord
POP BX         ;5b         A, Y-coord
POP CX         ;59   stick B, X-co-rd
POP DX         ;5a         B, Y-coord
POP BP         ;5D
STI            ;FB   enabled ints and return
RETF 0002      ;CA0200 