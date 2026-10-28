
		name    HPC_CD
		title   '!PC HPC CD-ROM device driver'
		page    ,132

; COMMENTS *************************************************************
;
;       Version 0.1     Written by: Ian Harvey
;
;       History
;       2.0	20-11-95 INH	New HPC version, based on A1CD.SYS
;
; GENERAL DEFINITIONS ****************************************************

	beep		equ	7
	cr              equ     13
	lf              equ     10
	eom             equ     '$'     ;End of message for strings

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
	stBUSYERR	equ	08302H
	stBADCMD	equ	08103H
	stRDERR		equ	0810BH
	stGENERR	equ	0810CH
	stCHANGED	equ	0810FH


; Some structures...

; Dword pointer/integer

DWORD_POINTER	STRUC
fpOFS		dw	0
fpSEG		dw	0
ENDS

DWORD_INTEGER	STRUC
loword		dw	0
hiword		dw	0
ENDS

; Request header

REQUESTHEADER   STRUC
rh_Length    db ?        ;length of record, in bytes
rh_Unit      db ?        ;unit number (block device only)
rh_Function  db ?        ;function number
rh_Status    dw ?        ;status
rh_Reserved  db 8 dup(?) ;reserved
ENDS

; Init command header

INITSTRUCT	STRUC
init_pad	db	13 dup (0)	; Request header
init_units	db	0		; Number of units (must be 0)
init_lastaddr	dd	?		; End address
init_cmdlineptr	dd	?		; Ptr to cmd line parameters
init_drvletter	db	0		; Block device number
ENDS

; Read long command (80h)

READSTRUCT	STRUC
read_pad	db	13 dup (0)	; Request header
read_amode	db	0	; Addressing mode
read_ptr	dd	?	; Data transfer address
read_count	dw	?	; Number of sectors to read
read_sect	DD	?	; Starting sector number
read_mode	DB	?	; Data read mode
read_ILsize	DB	?	; Interleave size (uh?)
read_ILskip	DB	?	; Interleave skip factor (wot?)
ENDS

; Play Audio command (84h)

PLAYSTRUCT	STRUC
play_pad	db	13 dup (0)	; Request header
play_amode	db	?	; Addressing mode
play_sect	DD	?	; Starting sector number
play_count	DD	?	; Number of sectors to read
ENDS

; IOCTL Read (03h)/Write(0Ch) commands

IOCTLSTRUCT	STRUC
ioctl_pad	db	13 dup (0)	; Request header
ioctl_mdb	db	0		; Media descriptor byte (=0)
ioctl_addr	dd	?		; Transfer address
ioctl_len	dw	?		; Number of bytes to transfer
ioctl_sectorno	dw	0		; Starting sector number (=0)
ioctl_volumeid	dd	0		; DWORD ptr to requested vol (=0)
ENDS

; IOCTL buffer formats ---------

IOCTLBUF	STRUC			; Generic IOCTL structure
ioctl_code	DB	0
ENDS

GETHDRSTRUCT	STRUC
geth_code	DB	0	; Control block code
geth_addr	DD	?	; Address of device header
ENDS

GETPLAYADDRSTR	STRUC
gpa_code	DB	0	; Control block code
gpa_amode	DB	0	; Addressing mode for result
gpa_addr	DD	?	; Current play address
ENDS

GETAUDIOCHSTR	STRUC
gch_code	DB	0	; Control block code
gch_ch0_src	DB	0	; Channel 0 source (0..3)
gch_ch0_vol	DB	0	; Channel 0 volume (0..FF)
gch_ch1_src	DB	0	; Channel 1 source (0..3)
gch_ch1_vol	DB	0	; Channel 1 volume (0..FF)
gch_ch2_src	DB	0	; Channel 2 source (0..3)
gch_ch2_vol	DB	0	; Channel 2 volume (0..FF)
gch_ch3_src	DB	0	; Channel 3 source (0..3)
gch_ch3_vol	DB	0	; Channel 3 volume (0..FF)
ENDS

READDRVBYTESSTR	STRUC
rdb_code	DB	0	; Control block code
rdb_len		DB	0	; Number of bytes returned
ENDS

GETDRVSTATSTR	STRUC
gds_code	DB	0	; Control block code
gds_status	DD	0	; 32-bit flags word
ENDS

GETSECTSIZESTR	STRUC
gss_code	DB	0	; Control block code
gss_rdmode	DB	0	; 0=cooked, 1=raw
gss_size	DW	0	; Sector size
ENDS

GETVOLSIZESTR	STRUC
gvs_code	DB	8	; Control block code
gvs_size	DD	?	; Volume size
ENDS

MEDIACHANGESTR	STRUC
chg_code	DB	9	; Control block code
chg_state	DB	?	; Media byte
ENDS

CHG_CHANGED	equ 0FFh
CHG_NOCHANGE	equ 01h

GETDISKINFOSTR	STRUC
gdi_code	DB	10	; Control block code
gdi_firsttrk	DB	?	; Lowest track number
gdi_lasttrk	DB	?	; Highest track number
gdi_leadout	DD	?	; Starting point of the lead-out track
ENDS

GETTRKINFOSTR	STRUC
gti_code	DB	11	; Control block code
gti_trkno	DB	?	; Track number
gti_trkstart	DD	?	; Starting point of the track
gti_flags	DB	?	; Track control information
ENDS

GETQINFOSTR	STRUC
gqi_code	DB	12	; Control block code
gqi_data	DB	10 dup (?)	; Data
ENDS

GETAUDIOSTATSTR	STRUC
gas_code	DB	15	; Control block code
gas_status	DW	?	; Audio status bits
				; Bit 0 is Audio Paused bit
				; Bits 1-15 are reserved
gas_playstart	DD	?	; Start of last Play or for next Resume
gas_playstop	DD	?	; End of last Play or for next Resume
ENDS

LOCK_STRUCT	STRUC
lock_code	db	0
lock_unlock	db	0
ENDS


; DRIVER DEFINITIONS ***************************************************

; Pointer block for HPC_ExecCall

HPC_CallParms	STRUC
Buff_flag	DW	0	; Buffer number (0=A, 1=B)
Send_Len_1	DW	0	; Data bytes sent (portion 1), length
Send_Ptr_1	DD	0	; Data bytes sent, pointer
Send_Len_2	DW	0
Send_Ptr_2	DD	0
Ret_Len_1	DW	0	; Data bytes to be returned (portion 1) length
Ret_Ptr_1	DD	0	; Data bytes returned, pointer to buffer
Ret_Len_2	DW	0
Ret_Ptr_2	DD	0
ENDS

; Debugging only! ----------
;
;HPC_DebugB_Port equ 0301h
;HPC_DebugW_Port equ 0300h
;
;TraceW MACRO val	; !!!
;	PUSH	DX
;	PUSH	AX
;	MOV	AX, val
;	MOV	DX, HPC_DebugW_Port
;	OUT	DX, AX
;	POP	AX
;	POP	DX
;ENDM

; HPC Service ID for CD

HPC_CD_ID	equ	0003h


; CDROM structures ------------------

HPC_CD_INIT equ	0000h

CD_INIT_PARAMS STRUC
hpc_id		dw	0
reason		dw	0
ENDS

CD_INIT_RESULT STRUC
status		dw	0
audiostatus	dw	0
ndrives		dw	0
ENDS

;  --------------- 

HPC_CD_READ equ	0001h

CD_READ_MAXSECTS equ	7	; 7 sectors in 16K buffer

CD_READ_PARAMS STRUC
rd_hpc_id	dw	0
rd_reason	dw	0
drvnum		dw	0
sect_count	dw	0
sect_startL	dw	0
sect_startH	dw	0
readahead	dw	0
ENDS

CD_READ_RESULT STRUC
rd_status	dw	0
rd_audiostatus	dw	0
ENDS

;  --------------- 

HPC_CD_SEEK equ	0002h

CD_SEEK_PARAMS STRUC
sk_hpc_id	dw	0
sk_reason	dw	0
sk_drvnum	dw	0
sk_sect_count	dw	0
sk_sect_startL	dw	0
sk_sect_startH	dw	0
ENDS

CD_SEEK_RESULT STRUC
sk_status	dw	0
sk_audiostatus	dw	0
ENDS

;  --------------- 

HPC_CD_PLAY equ	0003h

CD_PLAY_PARAMS STRUC
pl_hpc_id	dw	0
pl_reason	dw	0
pl_drvnum	dw	0
pl_pad2		dw	0
play_startL	dw	0
play_startH	dw	0
play_lenL	dw	0
play_lenH	dw	0
ENDS

CD_PLAY_RESULT STRUC
pl_status	dw	0
pl_audiostatus	dw	0
ENDS

;  --------------- 

HPC_CD_PLAYSTATUS equ	0004h

CD_PLAYSTATUS_PARAMS STRUC
ps_hpc_id	dw	0
ps_reason	dw	0
ps_drvnum	dw	0
ENDS

CD_PLAYSTATUS_RESULT STRUC
ps_status	dw	0
ps_audiostatus	dw	0
ps_playstartL	dw	0
ps_playstartH	dw	0
ps_playposL	dw	0
ps_playposH	dw	0
ps_playstopL	dw	0
ps_playstopH	dw	0
ENDS

;  --------------- 

HPC_CD_DISKINFO equ	0005h

CD_DISKINFO_PARAMS STRUC
di_hpc_id	dw	0
di_reason	dw	0
di_drvnum	dw	0
ENDS

CD_DISKINFO_RESULT STRUC
di_status	dw	0
di_audiostatus	dw	0
disksizeL	dw	0
disksizeH	dw	0
ENDS

;  --------------- 

HPC_CD_TRACKINFO equ	0006h

CD_TRACKINFO_PARAMS STRUC
ti_hpc_id	dw	0
ti_reason	dw	0
ti_drvnum	dw	0
trknum		dw	0
ENDS

CD_TRACKINFO_RESULT STRUC
ti_status	dw	0
ti_audiostatus	dw	0
trkstartL	dw	0
trkstartH	dw	0
firsttrk	db	0
lasttrk		db	0
trkflags	db	0
ENDS

TRKNUM_WHOLE_DISK equ 0FFFFh

;  --------------- 

HPC_CD_QCHANNEL	equ	0007h

CD_QCHANNEL_PARAMS STRUC
qc_hpc_id	dw	0
qc_reason	dw	0
qc_drvnum	dw	0
ENDS

CD_QCHANNEL_RESULT STRUC
qc_status	dw	0
qc_audiostatus	dw	0
qdata		db	10 dup (0)
ENDS

;  --------------- 

HPC_CD_CONTROL	equ	0008h

CONTROL_RESET	equ 0
CONTROL_CHANGED equ 1
CONTROL_PAUSE	equ 2
CONTROL_RESUME	equ 3
CONTROL_UNLOCK	equ 4
CONTROL_LOCK	equ 5
CONTROL_OPEN	equ 6
CONTROL_CLOSE	equ 7

CD_CONTROL_PARAMS STRUC
ct_hpc_id	dw	0
ct_reason	dw	0
ct_drvnum	dw	0
fncode		dw	0
ENDS

CD_CONTROL_RESULT STRUC
ct_status	dw	0
ct_audiostatus	dw	0
ENDS

; DEVICE HEADER *******************************************************


_TEXT   segment byte public 'CODE'

		assume cs:_TEXT

		org 0
		.386
					;The Device Header...
Header:
		dd      -1              ;link to next driver
DevType 	dw      isCHAR + isDOS30 + doesIOCTL
					;Device attribute word
		dw      Strat           ;Pointer to strategy routine
		dw      Intr            ;Pointer to interrupt routine
DevName 	db      'MSCD000 '	;Name of device
		dw	0		;Reserved
		db      0		;Drive letter (must be 0)
Units		db	0		;Number of units supported

; DATA AREA ***********************************************************

InstallOnceFlag	DW	0DC44h

RHPtr   	dd      ?               ; Storage for Request Header ptr

Ret_Code		dw	0		; Result code, set by CallAndTranslate

CallBlk		HPC_CallParms <>	; Used for HPC calls

Call_Params	db	32 dup(0)	; Call parameters
Call_Result	db	32 dup(0)	; Call results

; JUMP TABLES *********************************************************

		;Dispatch table for main commands

Cmd0Dispatch	DW	Init		; 00 = Install
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
CMD0_MAX equ 0Fh

		; Dispatch table for commands 80-88h

Cmd8Dispatch	DW	Read_Data	; 80 = Read
		DW	BadCommand	; 81
		DW	Read_Prefetch	; 82 = Read prefetch
		DW	Read_Seek	; 83 = Seek
		DW	PlayAudio	; 84 = Play audio
		DW	PauseAudio	; 85 = Stop audio
		DW	BadCommand	; 86 = (Write)
		DW	BadCommand	; 87 = (Write with verify)
		DW	ResumeAudio	; 88 = Resume audio
CMD8_MAX equ 88h

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
IORD_MAX equ 0Fh

		; IOCTL Write dispatch table

IOWR_Dispatch	DW	OpenTray	; 00 = Open drawer
		DW	LockUnlock	; 01 = Enable/disable eject
		DW	ResetAudio	; 02 = Stop audio
		DW	BadCommand	; 03
		DW	BadCommand	; 04 = (Write drive bytes)
		DW	CloseTray	; 05 = Close drawer
IOWR_MAX equ 05h


; STRATEGY ************************************************************

; This called by MSDOS with ES:BX = request header
  
Strat           proc far

	assume cs:_TEXT, ds:NOTHING, es:NOTHING
	MOV	RHPtr.fpOFS, BX
	MOV	RHPtr.fpSEG, ES
	RET

Strat           endp

; INTERRUPT ***********************************************************

; This is called to actually execute the request.

Intr            proc far        ;Device Interrupt routine
	PUSHA
	PUSH    DS
	PUSH    ES 

	CLD

	MOV     AX, CS                  ;Put DS:=CS to make data
	MOV     DS, AX                  ;addressible
	assume	ds:_TEXT, es:NOTHING

	; Pre-set HPC_CallParms with default values *********************

	MOV	AX, 0
	MOV	BX, offset DS:CallBlk

	MOV	[BX].Buff_flag, AX	; Use buffer A

	MOV	[BX].Send_Ptr_1.fpOFS, offset DS:Call_Params
	MOV	[BX].Send_Ptr_1.fpSEG, DS

	MOV	[BX].Ret_Ptr_1.fpOFS, offset DS:Call_Result
	MOV	[BX].Ret_Ptr_1.fpSEG, DS

	MOV	[BX].Send_Len_2, AX
	MOV	[BX].Ret_Len_2, AX

	; Get request header & dispatch call ****************************

	LES     DI, RHPtr		; Put ES:DI = request header

	MOV     AL, ES:[DI].rh_Function ; Get function code into AX
	MOV	AH, 0			; 
	MOV	DL, ES:[DI].rh_Unit	; DL = unit (drive) number
	MOV	DH, 0

	MOV	Call_Params.hpc_id, HPC_CD_ID
	MOV	Call_Params.drvnum, DX

	CMP	AL, 80h			; Function codes over 80h?
	JAE	Intr_Do8x

	; Functions 0..0Fh ----------

	CMP	AL, CMD0_MAX			; 
	JA	BadCommand

	MOV	SI, AX
	ADD	SI, SI
	CALL	Cmd0Dispatch[SI]
	JMP	short Intr_Exit

	; Functions 80-88h ----------

Intr_Do8x:
	CMP	AL, CMD8_MAX
	JA	BadCommand

	MOV	SI, AX
	SUB	SI, 80h
	ADD	SI, SI
	CALL	Cmd8Dispatch[SI]

	; ---------------------------

Intr_Exit:
	assume	DS:nothing

	LES     DI, CS:RHPtr		; Get back request header pointer
	MOV     ES:[DI].rh_Status, AX	; Return status

	POP     ES
	POP     DS
	POPA
	RET
Intr            endp

; Dispatch routines ***************************************************
;
; These points are entered with:
;
; AX=function code
; DS=CS=Code/Data seg
; ES:DI->request structure
; DX = drive number
;
; DS:BX points to CallBlk, filled in apart from Send_Len_1 & Ret_Len_1
; hpc_id and drvnum in the Call_Params header are set correctly.
;
; Should near-RET with AX = status value to return; no regs need be
; preserved
;
; IOCTL read and write dispatch points are similar, except that ES:DI
; points to the IOCTL read/write control block.

Dispatch	proc near
	assume	ds:_TEXT, es:NOTHING

	; Unsupported commands ----------------------

BadCommand:			; Unsupported commands
	MOV	AX, stBADCMD
	RET

	; No-op command -----------------------------

Done:				; No-op commands
	MOV	AX, stDONE
	RET

	; Initialise --------------------------------

Init:
	MOV	AX, 0
	XCHG	AX, InstallOnceFlag
	CMP	AX, 0DC44h
	JNE	BadCommand
	JMP	Install

	; IOCTL read --------------------------------

IOCTL_Read:			; Function 03 = IOCTL Read
				; Header [0E..11] contains a pointer
				; to control block
	LES	DI, ES:[DI].ioctl_addr

	MOV	AL, ES:[DI].ioctl_code
	MOV	AH, 0
	CMP	AL, IORD_MAX
	JA	BadCommand

	MOV	SI, AX
	ADD	SI, SI
	JMP	IORD_Dispatch[SI]

	; IOCTL write ------------------------------

IOCTL_Write:			; Function 0C = IOCTL Write
				; Header [0E..11] contains a pointer
				; to control block
	LES	DI, ES:[DI].ioctl_addr

	MOV	AL, ES:[DI].ioctl_code
	MOV	AH, 0
	CMP	AL, IOWR_MAX
	JA	BadCommand

	MOV	SI, AX
	ADD	SI, SI
	JMP	IOWR_Dispatch[SI]

	; Read Data -------------------------------

Read_Data:			; Seek/prefetch are treated the same here
	MOV	SI, offset DS:Call_Params
	MOV	[BX].Send_Len_1, size CD_READ_PARAMS
	MOV	[BX].Ret_Len_1, size CD_READ_RESULT

	; Set up parameters

	MOV	[SI].reason, HPC_CD_READ

	MOV	AX, ES:[DI].read_ptr.fpOFS	; Data transfer pointer
	MOV	CX, AX				; (normalised)
	AND	AX, 0Fh
	MOV	[BX].Ret_Ptr_2.fpOFS, AX

	MOV	AX, ES:[DI].read_ptr.fpSEG
	SHR	CX, 4
	ADD	AX, CX
	MOV	[BX].Ret_Ptr_2.fpSEG, AX

	MOV	AX, ES:[DI].read_count		; Sector count
	MOV	[SI].readahead, AX		; Total no. of sectors

	MOV	AL, ES:[DI].read_amode		; Get AL = address mode
	MOV	CX, ES:[DI].read_sect.loword	; (0=logical, 1=Red Book)
	MOV	BX, ES:[DI].read_sect.hiword	; and BX:CX = input format
	CALL	ConvertToLBA			; Ensure in LBA format
	MOV	[SI].sect_startL, CX
	MOV	[SI].sect_startH, BX

Read_Loop:
	MOV	AX, Call_Params.readahead	; # left to read
	CMP	AX, CD_READ_MAXSECTS
	JB	RD_10
	MOV	AX, CD_READ_MAXSECTS
RD_10:
	MOV	Call_Params.sect_count, AX	; 2K per sector,
	SHL	AX, 11				; Convert to length in bytes
	MOV	CallBlk.Ret_Len_2, AX

	CALL	CallAndTranslate		; Do call, get status
	JC	Read_Exit			; Problem? If so, exit

	MOV	AX, CallBlk.Ret_Len_2		; Get back AX = transfer length
	ADD	CallBlk.Ret_Ptr_2.fpOFS, AX	; adjust pointer value

	MOV	AX, Call_Params.sect_count	; Get back # of sectors
	ADD	Call_Params.sect_startL, AX	; Adjust start sect #
	ADC	Call_Params.sect_startH, 0

	SUB	Call_Params.readahead, AX	; Dec no. of sectors left
	JA	Read_Loop			; If more, keep going

	MOV	AX, stDONE
Read_Exit:
	RET

	; Seek --------------------------------------

Read_Seek:			; Seek/prefetch are treated the same here
Read_Prefetch:
	MOV	SI, offset DS:Call_Params
	MOV	[BX].Send_Len_1, size CD_SEEK_PARAMS
	MOV	[BX].Ret_Len_1, size CD_SEEK_RESULT

	MOV	[SI].reason, HPC_CD_SEEK
	MOV	AX, ES:[DI].read_count
	MOV	[SI].sect_count, AX

	MOV	AL, ES:[DI].read_amode	; 0 = LBA, 1 = redbook (MMSSFF)
	MOV	CX, ES:[DI].read_sect.loword
	MOV	BX, ES:[DI].read_sect.hiword
	CALL	ConvertToLBA		; Ensure in LBA format
	MOV	[SI].sect_startL, CX
	MOV	[SI].sect_startH, BX

	CALL	CallAndTranslate	; Do HPC call, get AX = status
	RET

	; Play Audio ---------------------------------

PlayAudio:			; Function 84h = Play audio
	MOV	SI, offset DS:Call_Params
	MOV	[BX].Send_Len_1, size CD_PLAY_PARAMS
	MOV	[BX].Ret_Len_1, size CD_PLAY_RESULT
	MOV	[SI].reason, HPC_CD_PLAY

	MOV	AL, ES:[DI].play_amode	; 0 = LBA, 1 = redbook (MMSSFF)
	MOV	CX, ES:[DI].play_sect.loword
	MOV	BX, ES:[DI].play_sect.hiword
	CALL	ConvertToLBA		; Ensure in LBA format
	MOV	[SI].play_startL, CX
	MOV	[SI].play_startH, BX

	MOV	AX, ES:[DI].play_count.loword
	MOV	[SI].play_lenL, AX
	MOV	AX, ES:[DI].play_count.hiword
	MOV	[SI].play_lenH, AX

	CALL	CallAndTranslate
	RET

	; General control functions --------------------------

PauseAudio:
	MOV	AX, CONTROL_PAUSE
	JMP	short GenericControl

ResumeAudio:
	MOV	AX, CONTROL_RESUME
	JMP	short GenericControl

ResetAudio:
	MOV	AX, CONTROL_RESET
	JMP	short GenericControl

OpenTray:
	MOV	AX, CONTROL_OPEN
	JMP	short GenericControl

CloseTray:
	MOV	AX, CONTROL_CLOSE
	JMP	short GenericControl

LockUnlock:
	MOV	AX, CONTROL_LOCK
	CMP	ES:[DI].lock_unlock, 1		; value = 00 unlock, 01 lock
	JE	GenericControl
	MOV	AX, CONTROL_UNLOCK

GenericControl:
	MOV	Call_Params.reason, HPC_CD_CONTROL
	MOV	Call_Params.fncode, AX
	MOV	[BX].Send_Len_1, size CD_CONTROL_PARAMS
	MOV	[BX].Ret_Len_1, size CD_CONTROL_RESULT

	CALL	CallAndTranslate
	RET

	; Get Header address IOCTL ------------------------

GetHeader:			; Function 00 = Get pointer to CDROM header
	MOV	ES:[DI].geth_addr.fpOFS, offset CS:Header
	MOV	ES:[DI].geth_addr.fpSEG, CS
	MOV	AX, stDONE
	RET

	; Get Play address IOCTL --------------------------

GetPlayAddr:
	MOV	Call_Params.reason, HPC_CD_PLAYSTATUS
	MOV	[BX].Send_Len_1, size CD_PLAYSTATUS_PARAMS
	MOV	[BX].Ret_Len_1, size CD_PLAYSTATUS_RESULT

	CALL	CallAndTranslate	; AX = status
	JC	GPA_exit

	MOV	AL, ES:[DI].gpa_amode
	MOV	CX, Call_Result.ps_playposL
	MOV	BX, Call_Result.ps_playposH
	CALL	ConvertFromLBA		; BX:CX in desired format
	MOV	ES:[DI].gpa_addr.loword, CX
	MOV	ES:[DI].gpa_addr.hiword, BX

	MOV	AX, Ret_Code		; Get back AX = status
GPA_Exit:
	RET

	; Get Audio Channel info --------------------------

AudioChanInfo:			; Function 04 = Read audio channel info
	LEA	DI, [DI].gch_ch0_src
	MOV	AX, 0FF00h
	STOSW			; gch_ch0_src = 0. gch_ch0_vol=FF
	INC	AX
	STOSW			; gch_ch1_src = 1. gch_ch1_vol=FF
	INC	AX
	STOSW			; gch_ch2_src = 2. gch_ch2_vol=FF
	INC	AX
	STOSW			; gch_ch3_src = 3. gch_ch3_vol=FF
	MOV	AX, stDONE
	RET

	; Read drive bytes IOCTL ------------------------------
	
ReadDrvBytes:			; Function 05 - Read drive bytes
	MOV	ES:[DI].rdb_len, 0	;Length of data = 0
	MOV	AX, stDONE
	RET

	; Get Drive Status IOCTL ------------------------------
	
GetDriveStatus:			; Function 06 = Get drive status
	; Returns a 32-bit status word as follows:
	; Bit 0	0	Door closed
	;	1	Door open
	; Bit 1	0	Door locked
	;	1	Door unlocked
	; Bit 2	0	Supports only cooked reading
	;	1	Supports cooked and raw reading
	; Bit 3	0	Read only
	;	1	Read/write
	; Bit 4	0	Data read only
	;	1	Data read and plays audio/video tracks
	; Bit 5	0	No interleaving
	;	1	Supports ISO-9660 interleaving using interleave size and skip factor
	; Bit 6	0	Reserved
	; Bit 7	0	No prefetching
	;	1	Supports prefetching requests
	; Bit 8	0	No audio channel manipulation
	;	1	Supports audio channel manipulation
	; Bit 9	0	Supports HSG addressing mode
	;	1	Supports HSG and Redbook addressing modes
	; Bit 10 0	Does CD-XA (needed to make photoCDs work)
	; Bit 11 0	Disc is present in drive
	; 	 1	No disc is present in drive
	; Bit 12 0	Doesn't support R-W sub-channels
	;	 1	Supports R-W sub-channels
	; Bit 13-31	0	Reserved (all 0)

	MOV	ES:[DI].gds_status.loword, 400h + 200h + 10h	; Set bits 4, 9 & 10
	MOV	ES:[DI].gds_status.hiword, 0h

	; Do HPC call to find out if there's a disk in the drive

	MOV	Call_Params.reason, HPC_CD_DISKINFO
	MOV	[BX].Send_Len_1, size CD_DISKINFO_PARAMS
	MOV	[BX].Ret_Len_1, size CD_DISKINFO_RESULT

	CALL	CallAndTranslate	; AX = status
	JC	GDS_Exit		; If not OK, exit

	MOV	BX, Call_Result.disksizeL	; Disk size is zero if the
	OR	BX, Call_Result.disksizeH	; drive is empty
	JNZ	GDS_exit
	OR	ES:[DI].gds_status.loword, 800h + 1h
				; If no disk, say door is open & drive empty
GDS_Exit:
	RET

	; Get sector size IOCTL -------------------------------

GetSectorSize:			; Function 07 = Get sector size
	MOV	AX, 2048	; 'Cooked' sector size
	CMP	ES:[DI].gss_rdmode, 0
	JZ	GSS_10
	MOV	AX, 2352	; 'Raw' sector size
GSS_10:	
	MOV	ES:[DI].gss_size, AX
	MOV	AX, stDONE
	RET

	; Get Disk Size IOCTL --------------------------------

GetDiskSize:			; Function 08 = Get disk volume size
	MOV	Call_Params.reason, HPC_CD_DISKINFO
	MOV	[BX].Send_Len_1, size CD_DISKINFO_PARAMS
	MOV	[BX].Ret_Len_1, size CD_DISKINFO_RESULT

	CALL	CallAndTranslate	; AX = status, BX = BUSY bit
	JC	GVS_exit

	MOV	BX, Call_Result.disksizeL
	MOV	ES:[DI].gvs_size.loword, BX
	MOV	BX, Call_Result.disksizeH
	MOV	ES:[DI].gvs_size.hiword, BX
GVS_Exit:
	RET

	; Get Disk Changed IOCTL -----------------------------

GetDiskChanged:			; Function 09 = Disk changed
	MOV	Call_Params.reason, HPC_CD_CONTROL
	MOV	Call_Params.fncode, CONTROL_CHANGED
	MOV	[BX].Send_Len_1, size CD_CONTROL_PARAMS
	MOV	[BX].Ret_Len_1, size CD_CONTROL_RESULT

	CALL	CallAndTranslate	; AX = status code

	MOV	ES:[DI].chg_state, CHG_NOCHANGE
	CMP	AL, (stCHANGED AND 255)	; Avoid busy bit
	JNE	GDC_Exit
	MOV	ES:[DI].chg_state, CHG_CHANGED
	MOV	AX, stDONE
GDC_Exit:
	RET

	; Get Audio Track Info IOCTL --------------------------

GetAudioInfo:			; Function 0A = Read audio information
	MOV	Call_Params.reason, HPC_CD_TRACKINFO
	MOV	Call_Params.trknum, TRKNUM_WHOLE_DISK

	MOV	[BX].Send_Len_1, size CD_TRACKINFO_PARAMS
	MOV	[BX].Ret_Len_1, size CD_TRACKINFO_RESULT

	CALL	CallAndTranslate
	JC	GAI_exit

	MOV	AL, Call_Result.firsttrk
	MOV	ES:[DI].gdi_firsttrk, AL
	MOV	AL, Call_Result.lasttrk
	MOV	ES:[DI].gdi_lasttrk, AL

	MOV	AL, 01		; Red book format
	MOV	CX, Call_Result.trkstartL
	MOV	BX, Call_Result.trkstartH	; BX:CX = disk size

	CALL	ConvertFromLBA	; Convert to MM/SS/FF address

	MOV	ES:[DI].gdi_leadout.loword, CX
	MOV	ES:[DI].gdi_leadout.hiword, BX

	MOV	AX, Ret_Code		; Get back status
GAI_exit:
	RET

	; Get track info IOCTL ----------------------------------

GetTrackInfo:			; Function 0B = Get information on track
	MOV	Call_Params.reason, HPC_CD_TRACKINFO
	MOV	[BX].Send_Len_1, size CD_TRACKINFO_PARAMS
	MOV	[BX].Ret_Len_1, size CD_TRACKINFO_RESULT

	MOV	AL, ES:[DI].gti_trkno
	MOV	AH, 0
	MOV	Call_Params.trknum, AX

	CALL	CallAndTranslate
	JC	GTI_exit

	MOV	AL, Call_Result.trkflags
	MOV	ES:[DI].gti_flags, AL

	MOV	AL, 01		; Red book format
	MOV	CX, Call_Result.trkstartL
	MOV	BX, Call_Result.trkstartH	; BX:CX = disk size

	CALL	ConvertFromLBA	; Convert to MM/SS/FF address

	MOV	ES:[DI].gti_trkstart.loword, CX
	MOV	ES:[DI].gti_trkstart.hiword, BX

	MOV	AX, Ret_Code		; Get back status
GTI_exit:
	RET

	; Get Q-channel info IOCTL ---------------------------

GetQChannel:			; Function 0C = Read Q-channel info
	MOV	Call_Params.reason, HPC_CD_QCHANNEL
	MOV	[BX].Send_Len_1, size CD_QCHANNEL_PARAMS
	MOV	[BX].Ret_Len_1, size CD_QCHANNEL_RESULT

	CALL	CallAndTranslate
	JC	GQC_exit

	MOV	SI, offset DS:Call_Result.qdata ; DS:SI->start
	LEA	DI, [DI].gqi_data	; ES:DI->dest
	CLD
	MOV	CX, 5			; 5 words of data to copy
	REP	MOVSW
GQC_exit:
	RET
	
	; Get audio status IOCTL ------------------------------

GetAudioStatus:			; Function 0F = Get audio status
	MOV	Call_Params.reason, HPC_CD_PLAYSTATUS
	MOV	[BX].Send_Len_1, size CD_PLAYSTATUS_PARAMS
	MOV	[BX].Ret_Len_1, size CD_PLAYSTATUS_RESULT

	CALL	CallAndTranslate
	JC	GAS_Exit

	MOV	AL, 01
	MOV	CX, Call_Result.ps_playstartL
	MOV	BX, Call_Result.ps_playstartH	; BX:CX = start pos

	CALL	ConvertFromLBA	; Convert to MM/SS/FF address

	MOV	ES:[DI].gas_playstart.loword, CX
	MOV	ES:[DI].gas_playstart.hiword, BX

	MOV	AL, 01
	MOV	CX, Call_Result.ps_playstopL
	MOV	BX, Call_Result.ps_playstopH	; BX:CX = stop pos

	CALL	ConvertFromLBA	; Convert to MM/SS/FF address
	
	MOV	ES:[DI].gas_playstop.loword, CX
	MOV	ES:[DI].gas_playstop.hiword, BX

	MOV	AX, 0
	CMP	Call_Result.audiostatus, 2	; Is audio paused?
	JNE	GAS_10
	MOV	AX, 1				; status = 1 if so
GAS_10:
	MOV	ES:[DI].gas_status, AX

	MOV	AX, Ret_Code
GAS_exit:
	RET

Dispatch	endp


	; Handy subroutines ***********************************


ConvertToLBA proc near ; -----------------------------

	; Used to convert an address to standard 'logical block address'
	; form. On entry, AL is the addressing mode
	;  0 = logical block address (CX is LS word, BX is MS word)
	;  1 = red book (BL is minute, CH is second, CL is frame)
	;
	; for address mode 0, we do nothing. Otherwise, we return
	;   LBA = min*60*75 + sec*75 + frame - 150
	;
	; Corrupts AX, DX

	CMP	AL, 1
	JE	CtoL_RedBook
	RET

CtoL_RedBook:
	MOV	BH, 0		; BX = minute
	MOV	AX, 60*75
	MUL	BX
	MOV	BX, AX		; DX:BX = min*60*75

	MOV	AL, 75
	MUL	CH		; AX = sec * 75
	MOV	CH, 0		; CX = frame
	ADD	AX, CX		; AX = sec*75 + frame
	ADD	AX, BX
	ADC	DX, 0		; DX:AX = min*60*75+sec*75+frame

	SUB	AX, 150
	SBB	DX, 0
	MOV	BX, DX
	MOV	CX, AX		; BX:CX = min*60*75+sec*75+frame-150

	RET
ConvertToLBA endp


ConvertFromLBA proc near ; -----------------------------

	; Used to convert a logical block address to standard
	; a given form. 
	; On entry, BX:CX is the input LBA, and AL is the desired result
	;  mode:
	;
	;  0 = logical block address (CX is LS word, BX is MS word)
	;  1 = red book (BL is minute, CH is second, CL is frame)
	;
	; Corrupts AX, DX

	CMP	AL, 1
	JE	CfromL_RedBook
	RET

CfromL_RedBook:

	;   LBA = min*60*75 + sec*75 + frame - 150
	;
	; So, we do:
	;  tmp = LBA+150;
	;  min = (tmp / 60*75);
	;  tmp =  tmp % 60*75;
	;  sec =  tmp / 75
	;  frame = tmp % 75

	MOV	DX, BX
	MOV	AX, CX
	ADD	AX, 150
	ADC	DX, 0		; DX:AX = LBA+150

	MOV	CX, 60*75
	DIV	CX		; AX  = tmp/60*75, DX = tmp % 60*75
	MOV	BX, AX		; BL = minute, BH should be 0.
	MOV	AX, DX
	MOV	CL, 75
	DIV	CL		; AL = tmp/75, AH = tmp % 75
	MOV	CH, AL		; CH = second
	MOV	CL, AH		; CL = frame
	RET	

ConvertFromLBA endp


CallAndTranslate proc near ; -------------------------

	assume DS:nothing, ES:nothing

	; This does an HPC CD call, and returns AX = an appropriate status
	; value. The returned 'audiostatus' value is used to generate a
	; BUSY bit. Carry is set if there is an error condition.
	; 'Ret_Code' is set to the value returned in AX.

	; Corrupts BX, CX, DX, SI. Preserves ES:DI

	MOV	SI, ES
	MOV	AX, CS
	MOV	ES, AX
	MOV	BX, offset CS:CallBlk
	MOV	AX, 0001h
	INT	4Dh
	MOV	ES, SI

	CMP	AX, 0			; Did HPC work?
	JNE	CAT_gen_fail		; If not, general failure

	MOV	BX, CS:Call_Result.status	; AX = status code
	CMP	BX, 7			; Max. recognised code
	JA	CAT_gen_fail
	ADD	BX, BX
	MOV	AX, CS:StatusXlate[BX]

	TEST	CS:Call_Result.audiostatus, 1	; Is audio playing?
	JZ	CAT_10
	OR	AX, stBUSY			; If so set BUSY bit
CAT_10:
	MOV	Ret_Code, AX
	CMP	AL, 0			; LSB is error code, 0=OK
	JNE	CAT_20
	CLC
	RET

CAT_gen_fail:
	MOV	AX, stGENERR	; If not, general failure error!
	MOV	Ret_Code, AX
CAT_20:
	STC
	RET

StatusXlate	dw	stDONE		; 0 = success
		dw	stGENERR	; 1 = Bad function code
		dw	stBADCMD	; 2 = bad drive no.
		dw	stBADCMD	; 3 = bad params
		dw	stCHANGED	; 4 = disk changed
		dw	stBUSYERR	; 5 = busy error
		dw	stRDERR		; 6 = data error
		dw	stGENERR	; 7 = drive error

	
CallAndTranslate endp

; END OF RESIDENT SECTION OF DRIVER ***********************************

NonResident:

; INITIALISATION ROUTINE **********************************************


	; Called by Interrupt routine when driver first installs

Install proc near
	assume	DS: _TEXT, ES:nothing

	MOV  AH, 9
	MOV  DX, offset DS:SignOn
	INT  21H

	; Get command line ***************

	PUSH	DS
	LDS	SI, ES:[DI].init_cmdlineptr
	CALL	GetCmdLine
	POP	DS

	MOV	DX, offset DS:ErrCmdLine
	JC	Inst_Fail

	; Check HPC services *************

	MOV	AX, 0
	MOV	ES, AX
	MOV	AX, word ptr ES:[4Dh*4]	; Get HPC Services vector
	OR	AX, word ptr ES:[4Dh*4 + 2]

	MOV	DX, offset DS:ErrHPCCD	; If it is, give error
	JZ	Inst_Fail

	; Do CD Init *********************

	MOV	BX, offset DS:CallBlk
	MOV	Call_Params.reason, HPC_CD_INIT
	MOV	[BX].Send_Len_1, size CD_INIT_PARAMS
	MOV	[BX].Ret_Len_1, size CD_INIT_RESULT

	CALL	CallAndTranslate	; Try CD initialise function

	MOV	DX, offset DS:ErrHPCCD	; Did it work?
	JC	Inst_Fail

	; Get no. of drives *********************

	MOV	AX, Call_Result.ndrives
	MOV	DX, offset DS:ErrNoCDs

	CMP	AX, 0
	JE	Inst_Fail

	MOV	Units, AL		; AL = no. of drives supported
	ADD	InstMsg[0], AL		; Amend install message

	MOV	AH, 9
	MOV	DX, offset InstMsg
	INT	21H

	; Install success **********************

	;If installed OK, set field
	;in request header for address of
	;free memory to start of the
	;installation routine (which is
	;therefore non-resident)

	LES  DI, RHPtr
	MOV  ES:[DI].init_lastaddr.fpOFS, offset CS:NonResident
	MOV  ES:[DI].init_lastaddr.fpSEG, CS
	MOV  ES:[DI].init_units, 0

	MOV	AX, stDONE
	RET

	; Install failure **********************

Inst_Fail:
	; Get here with DX = offset to error message
	MOV  AH, 9			; Show on screen
	INT  21H

	MOV  AH, 9
	MOV  DX, offset DS:NoInstMsg
	INT  21H

	; Set up pointers to abort
	; installation if device fails
	; claim we're a disk device with no units if it fails.

	LES  DI, RHPtr
					;Free memory = start of driver
	MOV  ES:[DI].init_lastaddr.fpOFS, 0
	MOV  ES:[DI].init_lastaddr.fpSEG, CS
	MOV  ES:[DI].init_units, 0
					;Pretend to be block device
	MOV  DevType, isBLOCK

	MOV  AX, stDONE
	RET
Install endp

	JMP	Intr_exit

; GetCmdLine routine ***************************************************

; Entered with DS:SI -> command line. Should preserve ES,DI
; Returns carry set if error.

GetCmdLine	proc near
	assume	DS:nothing, ES:nothing

	CLD

	; Get characters from command line ******************
GCL_Loop:
	LODSB			;Get next character from command line
GCL_Find:
	CMP	AL, '/'		;Is it a switch?
	JE	GCL_001
	CMP	AL, 32		;if it's a space or printable, ignore
	JAE	GCL_Loop

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
	MOV	DI, offset CS:DevName	; In device header
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
	STC
	RET

GetCmdLine	endp

; Device driver data ***************************************************

SignOn		db	"HPC_CD.SYS: CD-ROM driver for PC Cards", cr, lf
		db	"V2.0 29/Nov/95 (C) Aleph One Ltd. 1995", cr,lf,eom

InstMsg		db	"0 drive(s) available", cr, lf, eom

NoInstMsg	db	beep, "HPC_CD.SYS not installed"
		db	cr, lf, eom

ErrCmdLine	db	"ERROR! Invalid line in CONFIG.SYS - use", cr, lf, lf
		db	" DEVICE=HPC_CD.SYS /D:name", cr, lf, lf, eom

ErrHPCCD	db	"ERROR! Cannot find HPC CDROM services", cr, lf
		db	"This driver needs !PC version 2.00 or later"
		db	cr, lf, eom

ErrNoCDs	db	"** No CD-ROM drives working **"
		db	cr, lf, eom


	_TEXT	ends

	end

; END OF FILE *********************************************************

