
		name    A1WIN95
		title   '!PC Windows-95 support'
		page    ,132

; COMMENTS *************************************************************
;
;       Version 0.1     Written by: Ian Harvey
;
;       History
;       0.1     11-05-92 INH:	Original - just clears the screen
;
; GENERAL CONSTANTS ****************************************************

	beep		equ	7
	cr              equ     13
	lf              equ     10
	eom             equ     '$'     ;End of message for strings


;DRIVER CONSTANTS *****************************************************

	;Device Attribute Word

	isCHAR          equ     08000H
	isBLOCK         equ     0
	doesIOCTL       equ     04000H  ;can do IOCTL reads and writes
	isNonIBM        equ     02000H
	doesOPBSY       equ     02000H  ;CHAR only: can do Output-until-busy
	isDOS30         equ     00800H  ;supports DOS3.0 removable media
	isDOS32         equ     00040H  ;supports DOS3.2 generic IOCTL
	isBIG           equ     00002H  ;BLOCK only: 32-bit sector no.

	MaxCmd          equ     24      ;Max command number

	stERROR         equ     08000H
	stBUSY          equ     00200H
	stDONE          equ     00100H
	stBADCMD	equ	08103H
	stGENERR	equ	0810CH
	stRDERR		equ	0810BH


; DEVICE HEADER *******************************************************


	_TEXT   segment byte public 'CODE'

		assume cs:_TEXT, ds:_TEXT, es:NOTHING

		org 0
					;The Device Header...
	Header:
		dd      -1              ;link to next driver
	DevType dw      isBLOCK		;Device attribute word
		dw      Strat           ;Pointer to strategy routine
		dw      Intr            ;Pointer to interrupt routine
	DevName db      'A1WIN95 '	;Name of device
		dw	0		;Reserved
		db      0		;Drive letter (must be 0)
	Units	db	0		;Number of units supported

; GENERAL DATA ********************************************************

	RHPtr   dd      ?               ;Storage for Request Header ptr

; JUMP TABLES *********************************************************

		;Dispatch table for main commands

Cmd_Dispatch	DW	Install		; 00 = Install
		DW	BadCommand	; 01
		DW	BadCommand	; 02
		DW	BadCommand	; 03 = IOCTL Read
		DW	BadCommand	; 04
		DW	BadCommand	; 05
		DW	BadCommand	; 06
		DW	BadCommand	; 07 = Flush Input buffers
		DW	BadCommand	; 08
		DW	BadCommand	; 09
		DW	BadCommand	; 0A = Output status
		DW	BadCommand	; 0B = Flush output buffers
		DW	BadCommand	; 0C = IOCTL Write
		DW	BadCommand	; 0D = Device Open
		DW	BadCommand	; 0E = Device Close
		DW	BadCommand	; 0F

CMD_MAX equ 0Fh


; STRATEGY and INTERRUPT routines *************************************
  
Strat           proc far        ;Strategy routine
				;Called by MSDOS
				;with ES:BX = request header
	MOV  word ptr CS:[RHPtr],bx
	MOV  word ptr CS:[RHPtr+2],es
	RET

Strat           endp


Intr            proc far        ;Device Interrupt routine

	PUSH	AX                           ;Save all regs
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	SI
	PUSH	DI
	PUSH    DS
	PUSH    ES 
	
	MOV     AX, CS                  ;Put DS:=CS to make data
	MOV     DS, AX                  ;addressible

	; Get request header & organise parameters ********************

	LES     DI, [RHPtr]             ;Put ES:DI = request header

	MOV     AX, ES:[DI+01H]         ; Get device # in AL,
					; command code in AH
	CMP	AH, CMD_MAX
	JA	BadCommand

	MOV	BL, AH			; Get index into jump table
	AND	BX, 007Fh		; into BX
	SHL	BX, 1

	JMP	CS:Cmd_Dispatch[BX]

	; General service routines ****************

BadCommand:				; Generate an 'invalid command'
	MOV	AX, stBADCMD		; error
	JMP	Intr_Exit

Done:					; Return 'Done' status (i.e. ignore)
	MOV	AX, stDONE
	JMP	Intr_Exit
	
	; Exit ***********************

Intr_Exit:
	LES     DI, [RHPtr]             ; Get back request header
	MOV     ES:[DI+03h], AX         ; Return status

	POP     ES
	POP     DS
	POP	DI
	POP	SI
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	RET

Intr            endp

	; Service routines ***********************************

; END OF RESIDENT SECTION OF DRIVER ***********************************

NonResident:

; INSTALLATION ROUTINE ************************************************

	; Called by Interrupt routine when driver first installs

Install:
	MOV	AX, 0003h		; Set Text mode
	INT	10h

	MOV  AH, 9
	MOV  DX, offset SignOn
	INT  21H

Dont_stay_resident:
					;Set up pointers to abort
					;installation if device fails
	LES  DI, [RHPtr]
					;Free memory = start of driver
	MOV  word ptr ES:[DI+0EH], 0
	MOV  word ptr ES:[DI+10H], CS
					;Pretend to be block device
	MOV  [DevType], isBLOCK
					;No of devices = 0
	MOV  byte ptr ES:[DI+0DH], 0

	MOV  AX, stDONE
	JMP	Intr_exit

; Device driver data ***************************************************

SignOn		db	'PC Card Windows 95 support', cr, lf
		db	'V1.0 (C) Aleph One Ltd. 1995', cr,lf,eom

	_TEXT	ends

	end

; END OF FILE *********************************************************

