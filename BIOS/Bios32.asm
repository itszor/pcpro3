; ****************************************************************
;
; DIVA Project 80386 code
;
; BIOS32.ASM  Bits of the BIOS that want assembling in 32 bit instruction
;             mode (i.e. the VESA 2.0 protected mode interface)
;
;             It's mostly Ian's code copied here, and fiddled a bit
;
;       1997.02.24   MB  Original
;
; ****************************************************************

 INCLUDE	BIOSDefs.asi

        .486P
SVGA_PMI	SEGMENT USE32 PUBLIC
		ASSUME CS:SVGA_PMI

SVGA_PMI_table:
	DW	(SVGA_PMI_SetGetPage - SVGA_PMI_table)
	DW	(SVGA_PMI_SetGetDisplayStart - SVGA_PMI_table)
	DW	(SVGA_PMI_SetGetPalette - SVGA_PMI_table)
	DW	(SVGA_PMI_PrivsTable - SVGA_PMI_table)
SVGA_PMI_PrivsTable:
	DW 	3C7h, 3C8h, 3C9h, 300h, 301h ; I/O ports
	DW	0FFFFh
	; Memory addresses
	DW	0FFFFh

SVGA_PMI_ErrExit:
	POP	DS
	POP	ESI
	MOV	AX, 014Fh
	RET
SVGA_PMI_OKExit:
        POP     DS
        POP     ESI
	MOV	AX, 004Fh
	RET

SVGA_PMI_SetGetPage:
	PUSH 	ESI
	PUSH	DS
;	TRACEW	0abcdh
	JMP	SVGA_PMI_OKExit
SVGA_PMI_SetGetDisplayStart:
	PUSH	ESI
	PUSH	DS
	MOV	BX, DX
	MOV	DX, 3D4h
	MOV	AL, VESA_DISPSTART32
	MOV	AH, CL
	OUT	DX, AX
	MOV	AH, CH
	OUT	DX, AX
	MOV	AH, BL
	OUT	DX, AX
	MOV	AH, BH
	OUT	DX, AX
	JMP	SVGA_PMI_OKExit
	RET
SVGA_PMI_SetGetPalette:
; SVGA_SetGetPalette ------------------------------------

; On entry, BL=0 to set and 1 to get palette data
; bit 7 of BL is set to do it during a retrace (we ignore this)
; DX = first colour, CX = number of colours to update.
; ES:DI points to a table with 4 bytes per colour, in the
; sequence <ignored>, <red>, <green>, <blue>
;

SVGA_PMI_SetGetPalette:
	; Check values are in range, etc
	PUSH	ESI
	PUSH	DS

	CMP	DX, 255		; First colour
	JA	SVGA_PMI_SGP_Fail
	CMP	CX, 256		; Number of colours
	JA	SVGA_PMI_SGP_Fail	; Too many?
	JCXZ	SVGA_PMI_OKExit	; Nothing to do?

	MOV	AX, DX		; Last colour + 1
	ADD	AX, CX
	CMP	AX, 256
	JA	SVGA_PMI_SGP_Fail

	; On entry, BL=0 to set, BL=1 to get palette
	AND	BL, 07Fh
	CMP	BL, 01h
	JE	SVGA_PMI_GetPal_10
	JB	SVGA_PMI_SetPal_10
SVGA_PMI_SGP_Fail:
	JMP	SVGA_PMI_ErrExit

	; Set palette

SVGA_PMI_SetPal_10:
	MOV	AL, DL		; First colour to be written
	MOV	DX, 03C8h	; DAC-Write-Address register
	OUT	DX, AL

	MOV	ESI, EDI	; ES:SI points to table
	MOV	AX, ES
	MOV	DS, AX
	MOV	DX, 03C9h	; DAC data register
	CLD

SVGA_PMI_SP_loop:
	MOV	EBX, DS:[ESI]
	MOV	EAX, EBX
	SHR	EAX, 16
	OUT	DX, AL	; Send red
	MOV	EAX, EBX
	SHR	EAX, 8
	OUT	DX, AL ; Send green
	MOV	EAX, EBX
	OUT	DX, AL ; Send blue
	ADD	ESI, 4
	LOOP    SVGA_PMI_SP_loop
	JMP	SVGA_PMI_OKExit

	; Get palette

SVGA_PMI_GetPal_10:
	MOV	AL, DL		; First colour to be read
	MOV	DX, 03C7h	; DAC-Read-Address register
	OUT	DX, AL

	MOV	DX, 03C9h	; DAC Data register
	MOV	AL, 0		; "Ignore" value
	CLD

SVGA_PMI_GP_loop:
	STOSB			; Ignored byte
	INSB			; Get Red
	INSB			; Get Green
	INSB			; Get Blue
	LOOP	SVGA_PMI_GP_loop

	JMP	SVGA_PMI_OKExit
SVGA_PMI_table_end:

SVGA_PMI	ENDS

 END
