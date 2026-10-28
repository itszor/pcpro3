
		name    A1CD
		title   '!PC CD-ROM device driver'
		page    ,132

; COMMENTS *************************************************************
;
;       Version 0.1     Written by: Ian Harvey
;
;       History
;       0.1     11-05-92 INH:	Original
;	1.0	22-05-92	Amended to proper spec
;	1.1	08-11-94	Damn! Found a bug!!
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

; DEVICE-SPECIFIC CONSTANTS *******************************************

	devAttr         equ     isCHAR + isDOS30 + doesIOCTL

	CDRDrvSVC      MACRO
		PUSHF
		CALL	[SVCptr]
	ENDM

; DEVICE HEADER *******************************************************


	_TEXT   segment byte public 'CODE'

		assume cs:_TEXT, ds:_TEXT, es:NOTHING

		org 0
					;The Device Header...
	Header:
		dd      -1              ;link to next driver
	DevType dw      devAttr         ;Device attribute word
		dw      Strat           ;Pointer to strategy routine
		dw      Intr            ;Pointer to interrupt routine
	DevName db      'MSCD000 '	;Name of device
		dw	0		;Reserved
		db      0		;Drive letter (must be 0)
	Units	db	0		;Number of units supported

; GENERAL DATA ********************************************************

	RHPtr   dd      ?               ;Storage for Request Header ptr
	SVCptr	dd	0		;Pointer to SVC routine


; DEVICE-SPECIFIC DATA ************************************************

	ServiceID       DW      0001    ;to identify ARM driver


; JUMP TABLES *********************************************************

		;Dispatch table for main commands

Cmd0Dispatch	DW	Install		; 00 = Install
		DW	BadCommand	; 01
		DW	BadCommand	; 02
		DW	IOCTL_Read	; 03 = IOCTL Read
		DW	BadCommand	; 04
		DW	BadCommand	; 05
		DW	BadCommand	; 06
		DW	Done		; 07 = Flush Input buffers
		DW	BadCommand	; 08
		DW	BadCommand	; 09
		DW	BadCommand	; 0A = Output status
		DW	Done		; 0B = Flush output buffers
		DW	IOCTL_Write	; 0C = IOCTL Write
		DW	Done		; 0D = Device Open
		DW	Done		; 0E = Device Close
		DW	BadCommand	; 0F

		; Dispatch table for commands 80-88h

Cmd8Dispatch	DW	Read_Seek	; 80 = Read
		DW	BadCommand	; 81
		DW	Read_Seek	; 82 = Read prefetch
		DW	Read_Seek	; 83 = Seek
		DW	PlayAudio	; 84 = Play audio
		DW	SimpleCmd	; 85 = Stop audio
		DW	BadCommand	; 86 = (Write)
		DW	BadCommand	; 87 = (Write with verify)
		DW	SimpleCmd	; 88 = Resume audio

		; IOCTL Read dispatch table

IORD_Dispatch	DW	GetHeader	; 00 = Get pointer to driver header
		DW	GetPlayAddr	; 01 = Get current play address
		DW	BadCommand	; 02
		DW	BadCommand	; 03 = (Error statistics)
		DW	AudioChanInfo	; 04 = Read Audio Channel Info
		DW	ReadDrvBytes	; 05 = (Read drive bytes)
		DW	GetDriveStatus	; 06 = Get drive status
		DW	GetSectorSize	; 07 = Get sector size
		DW	GetDiskSize	; 08 = Get disk volume size
		DW	GetDiskChanged	; 09 = Disk changed
		DW	GetAudioInfo	; 0A = Read track info & size
		DW	GetTrackInfo	; 0B = Get information on track
		DW	GetQChannel	; 0C = Read Q-channel info
		DW	BadCommand	; 0D = (Read sub-channel)
		DW	BadCommand	; 0E = (Read UPC code)
		DW	GetAudioStatus	; 0F = Get play start addr

		; IOCTL Write dispatch table

IOWR_Dispatch	DW	SimpleCmd	; 00 = Open drawer
		DW	EnableEject	; 01 = Enable/disable eject
		DW	SimpleCmd	; 02 = Stop audio
		DW	BadCommand	; 03
		DW	BadCommand	; 04 = (Write drive bytes)
		DW	SimpleCmd	; 05 = Close drawer


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
	MOV	DX, ServiceID

	MOV	BL, AH			; Get index into jump table
	AND	BX, 007Fh		; into BX
	SHL	BX, 1

	TEST	AH, 80h
	JNZ	Intr_Do8x

	CMP	AH, 0Fh
	JA	BadCommand
	JMP	CS:Cmd0Dispatch[BX]

Intr_Do8x:
	CMP	AH, 88h
	JA	BadCommand
	JMP	CS:Cmd8Dispatch[BX]

	; General service routines ****************

BadCommand:				; Generate an 'invalid command'
	MOV	AX, stBADCMD		; error
	JMP	Intr_Exit

Done:					; Return 'Done' status (i.e. ignore)
	MOV	AX, stDONE
	JMP	Intr_Exit
	
SimpleCmd:				; Simple commands needing only
	CDRDrvSVC			; AH=command code, AL=device

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

IOCTL_Read:			; Function 03 = IOCTL Read
				; Header [0E..11] contains a pointer
				; to control block
	LES	DI, ES:[DI+0Eh]

	MOV	AH, ES:[DI]	; 1st byte of ctrl block = function code
	CMP	AH, 0Fh
	JA	BadCommand

	MOV	BL, AH
	AND	BX, 007Fh
	SHL	BX, 1
	ADD	AH, 40h		; CDRDrvSVC command code = IORD code + 40h
	JMP	CS:IORD_Dispatch[BX]

	; *******************************

IOCTL_Write:			; Function 0C = IOCTL Write
				; Header [0E..11] contains a pointer
				; to control block
	LES	DI, ES:[DI+0Eh]

	MOV	AH, ES:[DI]	; 1st byte of ctrl block = function code
	CMP	AH, 05h
	JA	BadCommand

	MOV	BL, AH
	AND	BX, 007Fh
	SHL	BX, 1

	ADD	AH, 60h		; CDRDrvSVC command code = IORD code + 60h
	JMP	CS:IOWR_Dispatch[BX]

	; *******************************

	; Enter with ES:[DI] = request header, AL = device #, AH = command

Read_Seek:			; Function 80, 82, 83 = Read, Prefetch or Seek
	MOV	BH, ES:[DI+0Dh]	; BH = address format (0=logical,1=RedBook)	
	MOV	CX, ES:[DI+12h]	; CX = # of blocks
	MOV	SI, ES:[DI+14h]	; SI = LSW of address
	MOV	BL, ES:[DI+16h]	; BL = MSB of address
	LES	DI, ES:[DI+0Eh] ; ES:DI = where to put it
	JMP	SimpleCmd	; Execute command		

	; *******************************

PlayAudio:			; Function 84 = Play audio
	MOV	BH, ES:[DI+0Dh]	; BH = address format (0=logical,1=RedBook)	
	MOV	SI, ES:[DI+0Eh]	; SI = LSW of start address
	MOV	BL, ES:[DI+10h]	; BL = MSB of start address
	MOV	CL, ES:[DI+14h]	; CL = MSB of length
	MOV	DI, ES:[DI+12h]	; DI = LSW of length
	JMP	SimpleCmd	; Execute command		


	; IOCTL Read dispatch routines ***********************

	; Enter with ES:DI = Control block, AH = CDRDrvSVC function code
	; AL = Device no, DX = Service ID

GetHeader:			; Function 00 = Get pointer to CDROM header
	MOV	word ptr ES:[DI+1], 0000h
	MOV	word ptr ES:[DI+3], CS
	MOV	AX, stDONE
	JMP	Intr_Exit

GetPlayAddr:			; Function 01 = Get current play address
	MOV	BH, ES:[DI+1]	; BH = address mode required
	CDRDrvSVC		; Do request
	MOV	ES:[DI+2], SI	; return SI = play address LSW
	MOV	ES:[DI+4], BX	; return BX = play address MSW
	JMP	Intr_Exit


AudioChanInfo:			; Function 04 = Read audio channel info
	MOV	AX, 0FF00h
	MOV	ES:[DI+1], AX	; Output 0 = channel 0, volume = 0FFh
	INC	AX
	MOV	ES:[DI+3], AX	; Output 1 = channel 1, volume = 0FFh
	INC	AX
	MOV	ES:[DI+5], AX	; Output 2 = channel 2, volume = 0FFh
	INC	AX
	MOV	ES:[DI+7], AX	; Output 3 = channel 3, volume = 0FFh
	MOV	AX, stDONE
	JMP	Intr_Exit
	
ReadDrvBytes:			; Function 05 - Read drive bytes
	MOV	byte ptr ES:[DI+1], 0h	;Length of data = 0
	MOV	AX, stDONE
	JMP	Intr_Exit
	
GetDriveStatus:			; Function 06 = Get drive status
	CDRDrvSVC		; Do request
	MOV	ES:[DI+01], SI	; SI = returned status 
	MOV	ES:[DI+03], BX	; BX = return status MSW
	JMP	Intr_Exit

GetSectorSize:			; Function 07 = Get sector size
	MOV	AX, 2048	; 'Cooked' sector size
	CMP	byte ptr ES:[DI+1], 0
	JZ	GSS_1
	MOV	AX, 2352	; 'Raw' sector size
GSS_1:	MOV	ES:[DI+2], AX
	MOV	AX, stDONE
	JMP	Intr_Exit

GetDiskSize:			; Function 08 = Get disk size
	CDRDrvSVC		; Do request
	MOV	ES:[DI+01], SI	; SI = returned size in blocks LSW
	MOV	ES:[DI+03], BX	; BX = returned size MSW
	JMP	Intr_Exit

GetDiskChanged:			; Function 09 = Disk changed
	CDRDrvSVC
	MOV	ES:[DI+01], BL	; BL = 01 changed, FF not changed
	JMP	Intr_Exit

GetAudioInfo:			; Function 0A = Read audio information
	CDRDrvSVC		; Do request
	MOV	ES:[DI+01], CX	; CL = first track #, CH = last track #
	MOV	ES:[DI+03], SI	; SI = returned size (Red-Book) LSW
	MOV	ES:[DI+05], BX	; BX = returned size MSW
	JMP	Intr_Exit

GetTrackInfo:			; Function 0B = Get information on track
	MOV	BH, ES:[DI+01]	; BH = track number
	CDRDrvSVC
	MOV	ES:[DI+02], SI	; SI = track start (Red-Book) LSW
	MOV	ES:[DI+04], BX	; BX = track start MSW
	MOV	ES:[DI+06], CL	; CL = track status bits
	JMP	Intr_Exit

GetQChannel:			; Function 0C = Read Q-channel info
	CDRDrvSVC		; Do request
	JMP	Intr_Exit
	
GetAudioStatus:			; Function 0F = Get audio status
	PUSH	BP
	CDRDrvSVC		; Do request
	MOV	ES:[DI+3], SI	; SI = Play start (Red-Book) LSW
	MOV	ES:[DI+5], BX	; BX = Play start MSW
	MOV	ES:[DI+7], CX	; CX = Play stop (Red-Book) LSW
	MOV	ES:[DI+9], DX	; DX = Play stop MSW
	MOV	ES:[DI+1], BP	; BP = 1 if paused, else 0
	POP	BP
	JMP	Intr_Exit


	; IOCTL Write dispatch routines **********************

EnableEject:			; Function 01 = Enable/disable eject
	MOV	BH, ES:[DI+1]	; BH = 00 Enable, 01 Disable
	JMP	SimpleCmd	; Do CDRDrvSVC and exit	

; END OF RESIDENT SECTION OF DRIVER ***********************************

NonResident:

; INSTALLATION ROUTINE ************************************************

	; Called by Interrupt routine when driver first installs

Install:
	MOV  AH, 9
	MOV  DX, offset SignOn
	INT  21H

	; Get command line ***************

	CALL	GetCmdLine
	JC	InstBad1

	; Get SVC vector *****************

	MOV	AX, 0B908h		; Int15h function B9h = Get SVC
					; vector (my invention)
	INT	15h
	JC	InstBad2
	CMP	AX, 0FFB9h		; Returns with ES:SI = vector
	JNE	InstBad2		; and AX = 0FFB9h for success

	MOV	word ptr [SVCptr],   SI
	MOV	word ptr [SVCptr+2], ES

	; Call Init function *********************

	MOV	AX, 0000h		; Install
	MOV	DX, ServiceID
	CDRDrvSVC
	CMP	BX, 0E6C8h		; BX must be E6C8 = identifier
	JNE	InstBad3

	CMP	AX, stDONE		; AX must be 'Done' status
	JNE	InstBad4
	MOV	[Units], CL		; CL = number of devices supported
	CMP	CL, 0			; No devices means error!
	JE	InstBad4

	ADD	[InstMsg], CL		; Amend 'drives installed' msg
	MOV	AH, 9
	MOV	DX, offset InstMsg
	INT	21H

Inst_OK:			;If installed OK, set field
				;in request header for address of
				;free memory to start of the
				;installation routine (which is
				;therefore non-resident)

	LES  DI, [RHPtr]
	MOV  word ptr ES:[DI+0EH], offset NonResident
	MOV  word ptr ES:[DI+10H], CS
	MOV  word ptr ES:[DI+03H], stDONE

	JMP	Intr_exit

	; Error exit routines *******************

InstBad1:
	MOV	DX, offset ErrMsg1
	JMP	Inst_Fail
InstBad2:
	MOV	DX, offset ErrMsg2
	JMP	Inst_Fail
InstBad3:
	MOV	DX, offset ErrMsg3
	JMP	Inst_Fail
InstBad4:
	MOV	DX, offset ErrMsg4

Inst_Fail:
	MOV  AH, 9
	INT  21H

	MOV  AH, 9
	MOV  DX, offset FailMsg
	INT  21H
					;Set up pointers to abort
					;installation if device fails
	LES  DI, [RHPtr]
					;Free memory = start of driver
	MOV  word ptr ES:[DI+0EH], 0
	MOV  word ptr ES:[DI+10H], CS
	MOV  word ptr ES:[DI+03H], stDONE
					;Pretend to be block device
	MOV  [DevType], isBLOCK
					;No of devices = 0
	MOV  byte ptr ES:[DI+0DH], 0

	JMP	Intr_exit

; GetCmdLine routine ***************************************************

; Entered with ES:DI = Request header

GetCmdLine	proc near

	PUSH DS
	LDS SI, ES:[DI+12H]	;Get Pointer to command line into
				;DS:SI

	CLD

	; Get characters from command line ******************
GCL_Loop:
	LODSB			;Get next character from command line
GCL_Find:
	CMP	AL, '/'		;Is it a switch?
	JE	GCL_001
	CMP	AL, 32		;if it's a space or printable, ignore
	JAE	GCL_Loop

	POP DS			;Else: exit
	CLC
	RET

	; Find switch *******

GCL_001:
	LODSB
	CMP	AL, 'D'
	JE	GCL_002
	CMP	AL, 'd'
	JNE	GCL_Error

GCL_002:
	LODSB
	CMP	AL, ':'
	JNE	GCL_Error

	; If '/D:' switch, copy device name ********

	MOV	AX, CS
	MOV	ES, AX			; Set ES:DI to point to device name
	MOV	DI, offset DevName	; In device header
	MOV	CX, 8

GCL_Loop2:
	LODSB				; Copy up to 8 characters or
	CMP	AL, 32			; the first whitespace/control
	JBE	GCL_Pad
	STOSB
	LOOP	GCL_Loop2		; Should be padded with spaces
	JMP 	GCL_Loop
GCL_Pad:
	MOV	AL, 32
	REP	STOSB
	JMP	GCL_Loop

	; Error exit ********

GCL_Error:
	POP DS
	STC
	RET

GetCmdLine	endp

; Device driver data ***************************************************

SignOn		db	'A1CD.SYS: CD-ROM driver for PC Cards', cr, lf
		db	'V1.1 (C) Aleph One Ltd. 1994', cr,lf,eom

InstMsg		db	'0 drive(s) available', cr, lf, eom

ErrMsg1		db	'ERROR! Invalid line in CONFIG.SYS - use', cr, lf, lf
		db	' DEVICE=A1CD.SYS /D:name', cr, lf, lf, eom

ErrMsg2		db	'ERROR! Cannot find DOS->RISCOS services', cr, lf
		db	"You are running this on a PC Card, aren't you?"
		db	cr, lf, eom

ErrMsg3		db	'ERROR! RISCOS CD-ROM support not loaded', cr, lf
		db	'Check your version of !PC'
		db	cr, lf, eom

ErrMsg4		db	'** No CD-ROM drives working **'
		db	cr, lf, eom

FailMsg		db	beep, 'A1CD.SYS not installed'
		db	cr, lf, eom

	_TEXT	ends

	end

; END OF FILE *********************************************************

