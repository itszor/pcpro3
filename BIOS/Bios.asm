	PAGE    ,132
; ****************************************************************
;
; DIVA Project 80386 code
;
; BIOS.ASM      Main BIOS Code - actual ROMable version
;               Copyright (C) Ian Harvey 1991-1995
;
; Versions
; 0.1   05-09-91 INH    Original
;        9-11-91        Test version: Does not read CMOS
;       21-12-91        Redundant code removed
;       10-01-92        Converted to 16Kbyte version
;       06-02-92        A20 line support
;       09-02-92        Include mouse support
;       25-03-92        32kbyte version
;       02-04-92        Sends Reset code to Arm_Comms_Status
;       03-04-92        For release 0.75
;       24-04-92        For 0.81
;       10-05-92        For 0.84
;       12-06-92        Cx486 test code
;       01-07-92        CMOS Things fixed; keyboard pause
;       20-07-92        For 0.94
;       30-07-92        For 0.95
;       26-09-92        Calls B386_Init
;       07-10-92        For 0.96
;       27-10-92        Checksum 'feature' fix
;       30-10-92        VGA Pointer Tables done
;       02-11-92        0.97 Release
;       04-11-92        Norton Editor fix in BIOSMOUS.ASI
;       18-11-92        1.01 Release BIOSSVC.ASI
;       06-01-93        1.03 restore SVC interrupts
;       11-01-93        1.04 Auto 486 test; CMOS setting in DSKinit
;       13-01-93        Final 1.04 release (486 test no present)
;       07-02-93        1.06 BIOSPRN fixed; multiple SVC Int bodge
;       16-02-93        1.08 486 test finished
;       01-03-93        1.09 Fonts moved round; Mouse data moved; better
;                         CGA cursor translation.
;       15-03-93        1.09c (for 8-bit card) No emulated I/O until shadow
;                         RAM initialised
;       26-03-93        1.10 Timeouts for parallel & serial ports
;       01-04-93        1.11 SYS functions added; intelligent port sensing
;       19-04-93        1.12  'Midnight' bug fix to BIOSTIC;
;       23-04-93        1.12a DOS 5 Install bug: Write TTY page fix
;       27-04-93        1.12c EMM386 font-move fix; mouse init
;       11-05-93        1.14  Shift-Tab fix; power saving
;       31-05-93        1.16 Video BIOS expansion
;       29-06-93        1.17 Acorn bug fixes
;       08-07-93        1.18 Function 11h done by BIOS
;       16-07-93        1.19- Video changes; Mode switch by BIOS
;       03-08-93        1.19 finished
;       23-08-93        1.20a 386Max fixes, RestoreBIOSState fix
;       26-08-93        1.21 ROM moving around. QEMM 6.02 fix
;       18-10-93        1.22 SVGA BIOS extensions
;       24-11-93        1.23 NoFloppyBoot. Turbo/Amouse fix. Print Screen.
;       29-11-93        1.23 BIOSInit mods
;       27-01-94        1.24 Real HD BIOS, Xtd BIOS data area
;       25-03-94        1.50 HD Startup timer; HD bugs fixed.
;       27-04-94        1.53 2nd serial port allowed
;       27-05-94        1.54 TI processor detection
;       14-06-94        1.55 Insert-key fixed, Copro int fixed
;       21-06-94        1.56 DMA Ch4 Enabled; TIC motorctrl, SVC int moved,
;                               HD reports error codes.
;       21-07-94        1.58 Some BIOSPRN fiddling. 16M RAM size fix.
;       03-08-94        1.59 48K ROM; floppy bits added; HDD/stubs changed to
;                               preserve D/I flags (etc). Timer only writes 3F2
;                               if floppies installed.
;       05-08-94             Flush pin enable added
;       02-11-94        1.63 Keyboard lights fix, proper enable/disable
;       14-11-94        1.64 Some EMM386 debug; better printer test; Int15/BAxx
;       29-11-94        1.65 New cache-enable code for Gemini;
;       30-11-94        1.66 Turn off Windows mode on startup; Fix Win mtask crash
;	07-12-94	1.67 A20gate delays & enabling; EMM386 xtd data fix;
;	13-12-94	1.68 A20gate now in software; i486SX/DX cache enabling
;	07-02-95	1.70 i486cpu option to force Intel-style cache setup
;	24-04-95	1.73 Out-of-sequence version - BIOS386.ASI for detecting
;				Cyrix/486DX2s was omitted from v1.75
;	24-04-95	1.76 Development version, new SVC command code
;	10-07-95	1.85 Keyboard bits fixed. Co-pro autodetect
;	12-07-95	1.86 Clock doubling SXL2 bit
;	25-08-95	1.89 Cache auto config
;	21-09-95	1.91 486DX4 and M1SC detection, A20gate frigging
;	17-10-95	(1.91) M1sc changed to 5x86
;	30-10-95	1.99 Beta test HPC version
;	07-12-95	1.991 Amouse fixed
;	25-01-96	1.993 Mono video bits deleted, CGA scroll fixed
;	28-02-96	1.994 HPC interrupt bugs fixed
;	24-04-96	1.995 Vid Write String 'null string' bug fixed,
;				TI 486 detection
;	08-05-96	2.00 Re-released as 2.00
;	03-07-96	2.01 VESA Stuff added
;     1997.03.12 W      2.04 Cyrix 5x86 config register address fixed
;     1997.04.11 W      2.04 extended scancodes (ctrl-up etc)
;     1997.06.13 W      2.06 STDev joystick support
;     1998.03.20 MB          VESA 2.0 support just about done
; *****************************************************************

	.286
Version         EQU     '2.06'
DateStamp       EQU     '13/06/97'      ; Back-to-front!!
;VendorName      EQU     'Aleph One'     ; not yet used
BIOSDataSeg     EQU     040H
BIOSXDataSeg    EQU     09FC0H
BIOSStackSeg    EQU     060H
BIOSRomSeg      EQU     0F400H
BIOSRomBase     EQU     (BIOSRomSeg- 0F000H) * 16
BIOSRomLength   EQU     0C000H

	INCLUDE BIOSDEFS.ASI
	INCLUDE BIOSDATA.ASI

	PUBLIC  prog_end

; Program text *****************************************************

BIOSCode        SEGMENT PARA PUBLIC 'CODE'

; File starts with identification message and ^Z ************

	ORG 0H

BIOS_SignOnMsg:

IFDEF PC_DEMO
        DB      "Aleph One Gemini PC Card BIOS (DEMO)", 13, 10, 10, 0
ELSE
	DB      "Aleph One Gemini PC Card BIOS", 13, 10, 10, 0
ENDIF
	DB      Version, ' ', DateStamp, 13, 10, 0
	DB      26

; At 080H is a table of ROM parameters **********************

	ORG 80H
CodeSeg@        DW      BIOSRomSeg              ;Seg address of ROM start
ROMlen          DW      (10000H - BIOSRomSeg)   ;length of ROM / 16bytes
MousePtr        DW      MOU_Int                 ;Used by ARCMOUSE.COM



; Start of power-on-reset code ****************************************

	ORG 100H

	ASSUME CS:BIOSCode, DS:BIOSData

	; Entry point after reset *************************************

BIOS_Start:

	CLI

	; Set SCAMP register values from preset table

	MOV     BX, offset CS:BIOS_SCAMPtbl

BIOS_SC_1:
	MOV     AX, CS:[BX]
	CMP     AL, 0FFH
	JZ      BIOS_SC_2
	OUT     IO_CFG_Index, AL

	MOV     AL, AH
	OUT     IO_CFG_Data, AL
	ADD     BX,2
	JMP     BIOS_SC_1

BIOS_SC_2:

	; Initialise timer channel 1 for refresh

	MOV     AL, 054h                ;Channel 1:mode 2
	OUT     IO_PIT_Cmd, AL
	MOV     AL, 012h                ;Time constant
	OUT     IO_PIT_Tmr1,AL


; Copy ROM to shadow RAM ***********************************

	MOV     AL, 011H                ; Address FEAXS register
	OUT     IO_CFG_Index, AL
	IOWAIT
	MOV     AL, 01011111B           ; Put shadow RAM into setup mode
	OUT     IO_CFG_Data, AL         ; Read=read ROM, Write=write RAM

					; Shadow RAM is loaded by copying
					; the relevant data area
					; onto itself.

	MOV     AX, CS                  ; Source (DS:SI)=start of ROM
	MOV     DS, AX                  ; Dest (ES:DI)=start of ROM
	MOV     ES, AX
	MOV     SI, 0
	MOV     DI, 0

	MOV     AX, DS:[SI]             ; Occasionally first
	MOV     ES:[DI], AX             ; word fails to transfer
	MOV     ES:[DI], AX

	MOV     CX, BIOSROMLength/2     ; Get number of words to copy

	CLD
	REP     MOVSW

	MOV     AL, 011H                ; Address FEAXS register
	OUT     IO_CFG_Index, AL
	MOV     AL, 10101111B           ; Put shadow RAM into operate mode
	OUT     IO_CFG_Data, AL         ; RAM up to EFFFFh

	IOWAIT
	IOWAIT

; General system initialisation ************************************

	; Disable NMIs

	MOV     AL, 80h                         ;Set
	OUT     IO_CMOS_Index, AL               ;port 70h bit 7

	; Give progress code to ARM
	XOUT    HPC_Cmd_Port, HPC_RESET_ACK

	; Set A20Gate low (i.e. 8086 mode)

	MOV	AL, 0D1h
	OUT	IO_Kbd_Cmd, AL		; Set A20gate command
	IOWAIT
	MOV     AL, 0DDh                ; bit1 = 0 = A20 gated off
	OUT     IO_Kbd_Data, AL
	IOWAIT
	IN	AL, IO_Kbd_Cmd		; Read status bit

	; Disable parity and I/O channel NMI's etc

	MOV     AL, 0Ch
	OUT     61h, AL

; Save warm-boot flag, zero RAM and set up stack ***********

	MOV     DS, CS:DataSeg@         ; Data starts at 00400h

	MOV     SS, CS:StackSeg@        ; Stack is at 006FEh down
	MOV     SP, 0100H

	MOV     BP, 0                   ; Test and clear warm boot flag
	XCHG    BP, [SYS_WarmBoot]

	CMP     BP, 04321H              ; Code to preserve RAM & do
	JNE     BIOS_WB_1               ; vectored jump
	JMP     [SYS_resetptr]

BIOS_WB_1:

	MOV     DI, 0                   ; Else, zero first 64K bytes
	MOV     ES, DI                  ; of RAM

	MOV     AX, 0
	MOV     CX, 07FFFh

	CLD
	REP STOSW

; Jump to warm or cold boot sequence ***********************

	CMP     BP, 01234H
	JNE     BIOS_ColdBoot
	JMP     BIOS_WarmBoot

; Cold Boot sequence ***********************************************

BIOS_ColdBoot:
	CALL    B386_Init
	CALL    SYS_Init
	CALL    HPC_Init		; Set up PC->ARM comms
	CALL    VID_Init

	;CALL	EXT_Init

	CALL    BIOS_SignOn

	CALL    KBD_Init
	CALL    TIC_Init
	CALL    SER_Init
	CALL    PRN_Init
	CALL    MOU_Init

	CALL    B486_SetupCPU

	CALL    RAM_ShowInfo
	CALL    RAM_ROMcheck
	CALL    RAM_MainRAMTest
	CALL    RAM_XtdRAMTest

	CALL	SYS_CoprocessorCheck

	CALL    FDD_Init
	CALL    HDD_Init
	INT     19H
	HLT

; Warm Boot sequence ***********************************************

BIOS_WarmBoot:
	CALL    B386_Init
	CALL    SYS_Init
	CALL    HPC_Init
	CALL    VID_Init

	CALL    BIOS_SignOn

	CALL    KBD_Init
	CALL    TIC_Init
	CALL    SER_Init
	CALL    PRN_Init
	CALL    MOU_Init

	CALL    B486_SetupCPU
	CALL    RAM_ShowInfo

	CALL	SYS_CoprocessorCheck

	CALL    FDD_Init
	CALL    HDD_Init
	INT     19H
	HLT

; Sign-on routine ********************************

BIOS_SignOn PROC NEAR
	MOV     SI, offset BIOS_SignOnMsg
	CALL    MSC_SendMsg
	RET

BIOS_SignOn ENDP

; Start-up info ***********************************************

	EVEN

DataSeg@        DW      BIOSDataSeg
StackSeg@       DW      BIOSStackSeg

BIOS_SCAMPtbl:
	DB      02H, 11111111B  ; SLTPTR: maximum value
	DB      0BH, 00000000B  ; EMSEN1: turn EMS off
	DB      05H, 00000011B  ; RAMSET:
	DB      06H, 00000000B  ; REFCTL: RAS only, 15.625uS
      ; DB      07H, 00000001B  ; CLKCTL:
	DB      13H, 00001001B  ; SLPCTL: /4 clock
	DB      14H, 00000101B  ; MISCSET: 16 bit IO decode
	DB      15H, 00101011B  ; ROMDMA:
	DB      16H, 00000011B  ; BUSCTL: 8 bit ROM width
	DB      0EH, 00000000B  ; ABAXS: all accesses to slots
	DB      0FH, 11111111B  ; CAXS: all as on-board RAM
	DB      10H, 11111111B  ; DAXS: all as on-board RAM

	DB      0FFH            ; End of table


; INCLUDEs for BIOS modules ****************************************

INCLUDE BIOSSYS.ASI

INCLUDE BIOSTIC.ASI

INCLUDE BIOSMOUS.ASI

INCLUDE BIOSPRN.ASI

INCLUDE BIOSSER.ASI

INCLUDE BIOSHPC.ASI

INCLUDE BIOSRAM.ASI

INCLUDE BIOSMSC.ASI

INCLUDE BIOSKBD.ASI

INCLUDE BIOSVID.ASI

INCLUDE BIOSVGA.ASI

INCLUDE BIOSVID2.ASI

INCLUDE BIOSSVGA.ASI

INCLUDE BIOSHDD.ASI

INCLUDE BIOSFDD.ASI

INCLUDE BIOS386.ASI

;INCLUDE BIOSEXT.ASI

prog_end        DB      0


; TABLES AND GLOBAL DATA ================================================

; ensure this appears at C000 when ROM loaded

	ORG     0C000h - BIOSRomBase

	DB      "Written by: Ian Harvey, Wookey and Matthew Bloch", 13, 10, 0
	DB      "Version ", Version, " dated ", DateStamp, 13, 10, 0
	DB      "Copyright (C) Aleph One 1991-1997", 13, 10, 0

	; Do BIOS tables and VGA fonts

	INCLUDE BIOSTBL.ASI
	INCLUDE BIOSFNTV.ASI

	; Ensure FE05B (POST entry point) is safe ..

	ORG     0E05Bh - BIOSRomBase
	LONGJMP BIOSRomSeg, BIOS_Start

	; Do EGA and CGA fonts

	INCLUDE BIOSFNTE.ASI
	INCLUDE BIOSFNTC.ASI

	; Various ROM entry points

	ORG     0F841h - BIOSRomBase
	LONGJMP BIOSRomSeg, SYS_RAMInt

	ORG     0F84Dh - BIOSRomBase
	LONGJMP BIOSRomSeg, SYS_EquipInt

	ORG     0F859h - BIOSRomBase
	LONGJMP BIOSRomSeg, SYS_Int15

	; Ensure CGA alt font lives at FFA6Eh

	ORG     0FA6Eh - BIOSRomBase
	INCLUDE BIOSFNTA.ASI

	; Miscellaneous entry points

	ORG     0FE6Eh - BIOSRomBase
	LONGJMP BIOSRomSeg, TIC_Int

	ORG     0FEA5h - BIOSRomBase
	LONGJMP BIOSRomSeg, TIC_Isr

	ORG     0FF53h - BIOSRomBase
	IRET                                    ; FFF53h is a dummy-IRET
BIOS_PrtScreen:
	LONGJMP BIOSRomSeg, PRN_PrintScreen     ; FFF54h is print-screen ISR


; Power on jump in boot ROM ****************************************

	; Ensure power-on jump lives at FFFF0h

	ORG     0FFF0H - BIOSRomBase

	LONGJMP BIOSRomSeg, BIOS_Start

	DB      DateStamp       ; Date of BIOS
	DB      0FFH, 0FCH, 000H        ; 0FCh, 00h = machine is AT

BIOSCode                ENDS

		END


