	PAGE	,132

COMMENT /*

	ARCMOUSE.ASM
	
	MASM source code for ARCMOUSE.COM

	This is used to redirect INT 33h to the mouse driver stored in ROM

	This is version 0.3 - backwards compatible with Acorn mouse

	*/


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

	MOV	AX, 0900H
	MOV	DX, offset CS:StartMessage
	INT	21H

	; Investigate ROM for 386PC mouse driver ********************

	PUSH	DS
	MOV	AX, 0F000h		; 0F000h = current ROM segment

checkloop:
	MOV	DS, AX

	CMP	DS:word ptr[080h], AX	; Offset 80h in our ROM should also
	JNE	check1			; contain segment address.

	MOV	DX, DS:word ptr [084h]	; Offset 84h is a pointer to ROM
	CMP	DX, 0			; mouse routine
	JNE	Found
check1:
	ADD	AX, 0200h		; Check in 4K lumps
	CMP	AX, 0
	JNE	checkloop

	POP	DS
	JMP	InstallAcorn		; If not found, install Acorn mouse

	; Found: install Driver & initialise mouse *****************

Found:
	MOV	AX, 02533h		; Function 25h = set INT vector
					; AL=33h= Mouse vector
					; DS:DX is vector to be set
	INT	21H			; Set the vector

	POP	DS

	MOV	AX, 0			; Then initialise the mouse
	INT	33h

	CMP	AX, 0FFFFh		; If AX <> 0FFFFh, the mouse is
	JNE	InitError		; not available.

	MOV	AX, 0900H
	MOV	DX, offset CS:OKMessage
	INT	21H

	MOV	AX,4C00H		;Terminate program
	INT	21H

	; Mouse did not initialise: give error ***********************

InitError:
	MOV	AX, 0900H		;Give error message
	MOV	DX, offset CS:ErrMessage
	INT	21H

	MOV	AX,4C01H		;Terminate program
	INT	21H

	; Install Acorn mouse driver *********************************

InstallAcorn:
	MOV	AX, 02533h		;Set interrupt vector
	MOV	DX, offset DS:AcornHandler
	INT	21h

	MOV	AX, 0			;Reset mouse code
	INT	33h

	CMP	AX, 0FFFFh
	JNE	InitError

	MOV	AX, 0900H
	MOV	DX, offset CS:AcornMessage
	INT	21H

	MOV	AX, 3100h		;Terminate and stay resident.
	MOV	DX, offset ResidentEnd	;Memory to reserve.
	ADD	DX, 15
	SHR	DX, 1
	SHR	DX, 1
	SHR	DX, 1
	SHR	DX, 1
	INT	21h			;Call DOS.


StartMessage	DB	'ARCMouse Archimedes mouse driver '
		DB	'V0.3 25-03-91', 0Dh, 0Ah, '$'

OKMessage	DB	'386PC Mouse Installed successfully'
		DB	0Dh, 0Ah, '$'

ErrMessage	DB	'Mouse not available - driver not installed'
		DB	0Dh, 0Ah, '$'

AcornMessage	DB	'Acorn PC-Emulator mouse driver installed'
		DB	0Dh, 0Ah, '$'

prog_end	DB	0

ProgCode		ENDS

		END	entrypoint
