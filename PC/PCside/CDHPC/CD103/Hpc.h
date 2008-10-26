/*************************************************************************
*
* Copyright (c) 1998 Aleph One Ltd
* 
* Module Name:  HPC.h
*
* definitions for HPC comms mechanism
*
*/



#define HPC_PORT_FIRST   0x300
#define HPC_PORTS_WIDTH	 7

#define HPC_Cmd_Port     0
#define HPC_Status_Port  0
#define HPC_Req_Port     2
#define HPC_DataA_Port   4
#define HPC_DataB_Port   6

#define HPC_IRQ_LINE  15

/* Extras defined locally */
#define HPC_DebugB_Port  (HPC_PORT_FIRST+0x1)
#define HPC_DebugW_Port  (HPC_PORT_FIRST+0x0)

/* Commands ------------------------------- */

#define HPC_WRITE_IO    0x00
#define HPC_READ_IO	0x10
#define HPC_WRITE_MEM	0x20
#define HPC_READ_MEM	0x30
#define HPC_DO_CALL	0x40
#define HPC_DO_CALL_IRQ	0x50
#define HPC_IRQ_NEEDED_BIT 0x10
#define HPC_RELEASE_BUF	0x60

#define HPC_BUFSEL_BIT 	   1
#define HPC_BUFFER_A 	   0
#define HPC_BUFFER_B 	   1

#define HPC_RESET_ACK	0x80
#define HPC_SUSPEND_ACK	0x82
#define HPC_MULTI_REQ	0x84
#define HPC_QUIT_REQ	0x86
#define HPC_FREEZE_REQ	0x88

/* Status given by ARM --------------------- */

#define HPCST_SUSPEND	0x80
#define HPCST_REQUEST   0x40
#define HPCST_READY     0x20
#define HPCST_LOCK_B	0x02
#define HPCST_LOCK_A	0x01


#define HPC_BUF_LEN 0x4000

/* macros to write to HPC ports using SCSIport driver calls */
#define HPC_WRITE_COMMAND(HPCBASE, VALUE) \
           ScsiPortWritePortUchar((HPCBASE+HPC_Cmd_Port), (UCHAR)(VALUE))
#define HPC_READ_STATUS(HPCBASE) \
           ScsiPortReadPortUchar(HPCBASE+HPC_Status_Port)
#define HPC_WRITE_REQUEST(VALUE) \
           ScsiPortWritePortUchar((HPCBASE+HPC_Req_Port), (VALUE))
#define HPC_READ_REQUEST(HPCBASE) \
           ScsiPortReadPortUchar(HPCBASE+HPC_Req_Port)
     /* these assume DataB port is dataA port+2 */      
#define HPC_WRITE_DATA(HPCBASE, VALUE, buffer) \
           ScsiPortWritePortUchar((HPCBASE+HPC_DataA_Port+buffer*2), (VALUE))
#define HPC_READ_DATA(HPCBASE, buffer) \
           ScsiPortReadPortUchar(HPCBASE+HPC_DataA_Port+buffer*2)

/* HPC command block describing data to transfer */

typedef struct _HPC_CALL_PARAMS
{
BYTE  Buff_flag;        /* 0 for buffer A 1 for buffer B */
BYTE  Interrupt_flag;   /* Set IRQ_NEEDED_BIT if needed */
WORD  Send_Len_1;       /* Send data - 1st portion: length */
PVOID Send_Ptr_1;       /*                         address */
WORD  Send_Len_2;       /*           - 2nd portion: length */
PVOID Send_Ptr_2;       /*                         address */
WORD  Ret_Len_1;      /* Return data - 1st portion: length */
PVOID Ret_Ptr_1;      /*                           address */
WORD  Ret_Len_2;      /*             - 2nd portion: length */
PVOID Ret_Ptr_2;      /*                           address */
} HPC_CALL_PARAMS;

/* HPC System calls structures ****************************************** */

typedef struct _HPC_SYSINFO_PARAMS
{
  WORD hpc_id;
  WORD reason;
} HPC_SYSINFO_PARAMS;

typedef struct _HPC_SYSINFO_RESULT
{
  ULONG identifier;
  BYTE  ver_minor;
  BYTE  ver_major;
  BYTE  pad1;
  BYTE  pad2;
  ULONG buf_A_physaddr;
  ULONG buf_B_physaddr;
} HPC_SYSINFO_RESULT;

typedef union
{
  struct _HPC_SYSINFO_PARAMS In;
  struct _HPC_SYSINFO_RESULT Out;
}
  HPC_SYSINFO_BLK;



//; Pointer block for HPC_ExecCall from CD.ASI
//HPC_CallParms	STRUC
//Buff_flag	DW	0	; Buffer number (0=A, 1=B)
//Send_Len_1	DW	0	; Data bytes sent (portion 1), length
//Send_Ptr_1	DD	0	; Data bytes sent, pointer
//Send_Len_2	DW	0
//Send_Ptr_2	DD	0
//Ret_Len_1	DW	0	; Data bytes to be returned (portion 1) length
//Ret_Ptr_1	DD	0	; Data bytes returned, pointer to buffer
//Ret_Len_2	DW	0
//Ret_Ptr_2	DD	0
