; code required for v1.2 of PCSleep
.286

MODEL TINY

SEGMENT pcsleep PARA 
codeseg

ORG 100h

;      ASSUME CS:pcsleep
start:
   CLI
   MOV AX, 0D000h
   MOV DS, AX
   MOV WORD PTR DS:[0000h], DX  ; show me what the DX value is
   AND DX, 0FE0h    ; AMD 5x86 has DX=4Ex (WT cache) or 4Fx (WB) on reset
   CMP DX, 04E0h    ; so mask out bit 4 as we don't care about that
   JE halt          ; if AMD skip CCR stuff as it doesn't like it
   MOV AL, 0C1h     ; set cyrix config registers.
   OUT 22h, AL
   MOV AL, 0
   OUT 23h, AL
   MOV AL, 0C2h
   OUT 22h, AL
   MOV AL, 38h
   OUT 23h, AL
halt:
   HLT

ENDS ;pcsleep

END start
