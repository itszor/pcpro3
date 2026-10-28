	PAGE	,132

COMMENT /*

	ARCMOUSE.ASM
	
	MASM source code for ARCMOUSE.COM

	This is used to redirect INT 33h to the mouse driver stored in ROM

	0.3 Backwards compatible with Acorn mouse
	0.4 Uses Int 15h function B9h
	0.5 Remove 386-specific reference
	*/


BEEP	equ	07h
CR	equ	0Dh
LF	equ	0Ah
EOM	equ	'$'

ProgCode	SEGMENT PARA PUBLIC 'CODE'

	ORG 100H


COMMENT /*
	Program Entry Point:
	*/

	ASSUME CS:ProgCode,DS:ProgCode,ES:ProgCode

entrypoint:
	JMP	Install

AcornHandler PROC FAR
	STI
	DW	-1, 33h		;SVC pseudo op code to enter emulator.
	IRET
	DW	-1, 104h	;SVC pseudo op code for return handler.  
AcornHandler ENDP

ResidentEnd:

	; Install the mouse handler *********************************

Install:
	PUSH	CS
	POP	DS

	MOV	ESval, ES

	MOV	AH, 09H
	MOV	DX, offset CS:StartMessage
	INT	21H

	; Investigate ROM for 386PC mouse driver ********************


	MOV	AX, 0B902h		; In15 function B9h (my invention)
	INT	15h			; is 'get SVC vector'. AL=02 =
	JC	TryAcorn		; mouse vector (although not done
	CMP	AX, 0FFB9h		; as an ARM SVC in this implementaion).
	JNE	TryAcorn		; Returns AX=0FFB9: success
					; ES:SI = vector

	; Found: install Driver & initialise mouse *****************

	PUSH	DS
	MOV	AX, ES
	MOV	DS, AX
	MOV	DX, SI

	MOV	AX, 02533h		; Function 25h = set INT vector
					; AL=33h= Mouse vector
					; DS:DX is vector to be set
	INT	21H			; Set the vector

	POP	DS

	MOV	AX, 0			; Then initialise the mouse
	INT	33h

	CMP	AX, 0FFFFh		; If AX <> 0FFFFh, the mouse is
	JNE	InitError		; not available.

	MOV	AH, 09H
	MOV	DX, offset CS:OKMessage
	INT	21H

	MOV	AX,4C00H		;Terminate program
	INT	21H

	; Mouse did not initialise: give error ***********************

InitError:
	MOV	AH, 09H			;Give error message
	MOV	DX, offset CS:ErrMessage
	INT	21H

	MOV	AX,4C01H		;Terminate program
	INT	21H

	; Try for Acorn mouse driver *********************************

TryAcorn:
	MOV	AX, 0FE00h		;Starting in ROM
	MOV	ES, AX			;absolute address FE000
	MOV	DI, 0			;attempt to find the word 'Acorn'
	MOV	CX, 0400h
	CLD
	MOV	AL, 'A'
SearchLoop:
	REPNE	SCASB			; Find an 'A' (DI->next char)
	JCXZ	NotAcorn		; If found, try to get
	CMP	ES:[DI], 06F63h		; 'co' characters (sleazy eh?!!)
	JNE	SearchLoop
	CMP	ES:[DI+2], 06E72h	; 'rn' characters
	JNE	SearchLoop

	; Acorn Machine ************************

ItsAcorn:
	MOV	AX, 02533h		;Set interrupt vector
	MOV	DX, offset DS:AcornHandler
	INT	21h

	MOV	AX, 0			;Reset mouse code
	INT	33h

	CMP	AX, 0FFFFh
	JNE	InitError

	MOV	AH, 09H
	MOV	DX, offset CS:AcornMessage
	INT	21H

	MOV	AX, 3100h		;Terminate and stay resident.
	MOV	DX, offset ResidentEnd	;Memory to reserve.
	ADD	DX, 15
	SHR	DX, 1
	SHR	DX, 1
	SHR	DX, 1
	SHR	DX, 1
	MOV	ES, ESval
	INT	21h			;Call DOS.

ESval		DW	0


	; Not Acorn machine *********************

NotAcorn:
	MOV	AH, 09H			;Give error message
	MOV	DX, offset CS:MachineMessage
	INT	21H

	MOV	AX,4C01H		;Terminate program
	INT	21H
	
StartMessage	DB	'ARCMouse Archimedes mouse driver '
		DB	'V0.5 02-11-92', CR, LF, EOM

OKMessage	DB	'Aleph One Mouse Installed successfully'
		DB	CR, LF, EOM

ErrMessage	DB	'Mouse not available - driver not installed'
		DB	CR, LF, EOM

AcornMessage	DB	'Acorn PC-Emulator mouse driver installed'
		DB	CR, LF, EOM

MachineMessage	DB	BEEP, CR, LF, 'ERROR! This driver runs only on '
		DB	"Acorn's PC Emulator or Aleph One's 386/486 card"
		DB	CR, LF, EOM

prog_end	DB	0

ProgCode		ENDS

		END	entrypoint
