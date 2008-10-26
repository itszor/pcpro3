/*************************************************************************
* 
* Copyright (c) 1997 Aleph One Ltd
* 
* Module Name:  CDHPC.c
* 
* Miniport Driver for Aleph One PC cards HPC mechanism - CDROM variant
* 
* Environment:  kernel mode only
* 
*       1997.11.18 W   Started from win95 sample miniport block driver code
* v0.01 1998.03.31     Working CDROM driver - no Audio support, no Sharedmem, no interrupts
* v0.02 1998.04.03     Sharedmem support, number of CD's correctly recognised, and disk change works
* v0.03 1998.04.08     Caching version - reads ahead up to 7 blocks. Eject and Lock/unlock supported
* v1.00 1998.04.08     Cachine removed due to 4K limit problem. Resources added
* v1.01 1998.04.10     Uses IDs instead of Luns to get round SCSIport driver bug.
* v1.02 1998.04 15     Now statically linked with memcpy to speed up transfers
*       1998.04.17     Rationalised status return stuff
* v1.03 1998.07.07     Added subQ channel support (for Autoroute)
* v1.04 1998.07.10     Fixed ID of audio disks

*
*Speed tests: Copying Greeting.Avi (7.95Mb)
* DOS CD driver: 30s
* v0.01          48s
* v0.02          38s
* v0.03          38s
* v1.02          30s :-) (with free libraries 29s)
*
*/

/* set this to 1 if interrupts are to be used, 0 if not - polling only for now*/
#define INTERRUPTFLG 0 
/* set this to 1 to enable read-ahead. Currently disabled due to '4k' problem*/
#define CACHING 0
/* turn debugging calls on and off  - normally set in build environment...*/
//#define DBG 0

//windows stuff
#include "miniport.h"
#include "memory.h"  //for memcpy
#include "stdarg.h"
#include "scsi.h"

//aleph1 stuff
#include "stdtypes.h" //this needs to come before other A1 files
#include "cdhpc.h"   
#include "hpc.h"   


//fns to generate 2 and 4 byte ints from SCSI blocks. Give it number of byte to work from.
#define MKWORDCDB(n) ( ((Srb->Cdb[n])<<8) + (Srb->Cdb[n+1]) )    /* Little endian ! */
#define MKLONGCDB(n) ( ((Srb->Cdb[n])<<24) + ((Srb->Cdb[n+1])<<16) + ((Srb->Cdb[n+2])<<8) + (Srb->Cdb[n+3]) )

#define MKWORD(a,b)     ( ((b)<<8)  +  (a) )    /* Little endian ! */
#define MKLONG(a,b,c,d) ( ((d)<<24) + ((c)<<16) + ((b)<<8) + (a) )

//macros for getting bytes from values. Byte 1 is LSB of 32bit word. Byte 4 is MSB
#define BYTE1(a) ((UCHAR) ( (a)        & 0xFF))
#define BYTE2(a) ((UCHAR) (((a) >>  8) & 0xFF))
#define BYTE3(a) ((UCHAR) (((a) >> 16) & 0xFF))
#define BYTE4(a) ((UCHAR) (((a) >> 24) & 0xFF))

//
// Logical Unit states.
//
typedef enum _LU_STATE {

   LS_NOTINITED,
   LS_INITED,
   LS_SELECT,
   LS_COMMAND,
   LS_DATA,
   LS_STATUS,
   LS_MSG_IN,
   LS_MSG_OUT,
   LS_COMPLETE

} LU_STATE, *PLU_STATE;

#define BLOCK_NOT_VALID 0xFFFFFFFF

/*typedef struct _CDBLOCK {
  UCHAR   Block[CD_BLOCK_SIZE];
} CDBLOCK; not used..*/

//ControlStatus flags:
#define PREVENT_REMOVAL 0x01
#define PREVENT_ACCESS  0x02
#define CD_CHANGED      0x03

//
// Logical Unit extension
//
typedef struct _SPECIFIC_LU_EXTENSION {

   LU_STATE            LuState;            // State information.
   UCHAR               LastAccessStatus;   // returned CD status on last commmand.
   UCHAR               LastAccessAudioStat;// returned CD Audio status on last commmand.
   ULONG               SavedDataPointer;   // Current data pointer.
   ULONG               SavedDataLength;    // Current data lenght.
   SENSE_DATA          SenseData;          // 18-byte sense data block
   MODE_PARAM_PAGES    ModePages;          // Loads of config info
   ULONG               DiskSize;           // Length of disk data
   PSCSI_REQUEST_BLOCK ActiveLuRequest;    // Active Srb for this LUN.
   UCHAR               ControlStatus;      // flags for CD Control
   UCHAR               AudioStatus;        // status of last/current audio command
  #if CACHING
   ULONG               CacheBlock[CD_CACHE_SIZE]; //sector numbers of cached data. 0xFFFFFFFF if not valid.
   UCHAR               CacheData[CD_CACHE_SIZE*CD_BLOCK_SIZE];  // Cached CD data, in blocks of 2K
  #endif

} SPECIFIC_LU_EXTENSION, *PSPECIFIC_LU_EXTENSION;

//
// Device extension
//
typedef struct _SPECIFIC_DEVICE_EXTENSION {

   PUCHAR              BaseAddress;        // Port address of adaptor.
   PUCHAR              MemAddress[2];      // Memory buffer addresses of adaptor
   BOOLEAN             SharedmemAvailable; // Set if using shared memory, not ports
   UCHAR               HPCstatus;          // Current HPCstatus
   ULONG               CurDataPointer;     // Current pointer for active LUN.
   ULONG               CurDataLength;      // Bytes left to xfer to this LUN.
   PSPECIFIC_LU_EXTENSION  ActiveLu;       // Currently active LUN.
   UCHAR               PathId;             // SCSI bus in use
   BOOLEAN             InterruptPending;   // Waiting for interrupt.
   UCHAR               NumCDDrives;        // Number of CDROM drives found

} SPECIFIC_DEVICE_EXTENSION, *PSPECIFIC_DEVICE_EXTENSION;


//
// Function declarations
//

ULONG DriverEntry( IN PVOID DriverObject, IN PVOID Argument2 );

ULONG CDHPCFindAdapter(
   PSPECIFIC_DEVICE_EXTENSION              DeviceExtension,
   IN PVOID                                Context,
   IN PVOID                                BusInformation,
   IN PCHAR                                ArgumentString,
   IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
   OUT PBOOLEAN                            Again
   );

ULONG CDHPCDetermineInstalled(
   PSPECIFIC_DEVICE_EXTENSION              DeviceExtension,
   IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
   IN OUT PULONG                           AdapterCount,
   OUT PBOOLEAN                            Again
   );

VOID CDHPCTimer( IN PVOID Context );

BOOLEAN CDHPCInitialize( IN PVOID Context );

BOOLEAN CDHPCStartIo( IN PVOID Context, IN PSCSI_REQUEST_BLOCK Srb );

BOOLEAN CDHPCInterrupt( IN PVOID Context );

BOOLEAN CDHPCResetBus(   IN PVOID Context, IN ULONG PathId );

VOID CDHPCAbort(
   IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
   IN PSPECIFIC_LU_EXTENSION     LuExtension,
   IN PSCSI_REQUEST_BLOCK        Srb
   );

VOID HPCCmdSetup(
   IN OUT HPC_CALL_PARAMS *HPCCommandBlk,
   IN     UCHAR            interruptflag,   IN     UCHAR            bufferflag,
   IN     WORD             Send1Length,     IN     PVOID            Send1,
   IN     WORD             Send2Length,     IN     PVOID            Send2,
   IN     WORD             Return1Length,   IN     PVOID            Return1,
   IN     WORD             Return2Length,   IN     PVOID            Return2
   );

ULONG HPCExecCall( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN HPC_CALL_PARAMS HPCCmdBlk );

VOID HPC_do_n_wait( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, BYTE buffer, BYTE useinterruptflag );

BYTE HPC_CheckStatus ( PSPECIFIC_DEVICE_EXTENSION  deviceExtension );
      
VOID CDHPCDoPlayAudio     ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoModeSense     ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoRead          ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoRealRead      ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoTestUnitReady ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoInquiry       ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );
VOID CDHPCDoReadTOC       ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension, IN OUT PSCSI_REQUEST_BLOCK Srb );


#if DBG

//
// Globals and externals used for debugging.
//

//
// CDHPCDebug affects which debug prints are enabled:
//
//     0x001   Arbitration and selection
//     0x002   Command, status
//     0x004   Data transfer
//     0x008   Status info from commands
//     0x010   Miniport entry points and completion
//     0x020   Initialization and interrupt
//     0x040   StatusCheck and WaitForRequest
//     0x080   Control register manipulation
//     0x100   Completion
//     0x200   Phase info
//     0x400   Temporary
//     0x800   Panic
//


ULONG CDHPCDebug = ( 0xFF8 );

#define HPCDebugPrint(MASK, ARGS)    \
       if (MASK & CDHPCDebug) {    \
       ScsiDebugPrint ARGS;    \
       }

#else

#define HPCDebugPrint(MASK, ARGS)

#endif


//CDformat translation routines
/* This function converts a physical block address into a Red Book
   address. The returned int value has frame no. in bits 0..7, seconds
   in bits 8..15, and minutes in bits 15..23
*/

static int PhysToMSF ( int addr )
{
  int mm, ss, ff;

  ss = (addr/75);
  ff = addr - (ss*75);
  mm = (ss/60);
  ss = ss - (mm*60);

  return MKLONG ( ff, ss, mm, 0 );

}

/* This function converts an MSF address into a Physical block address
   The supplied int value has frame no. in bits 0..7, seconds
   in bits 8..15, and minutes in bits 15..23.
*/

static int MSFtoPhys ( int addr )
{
  return (  (BYTE1(addr) * 60 * 75) + (BYTE2(addr) * 75) + BYTE3(addr) );
}

/* This function converts a logical block address into a Red Book
   address. The returned int value has frame no. in bits 0..7, seconds
   in bits 8..15, and minutes in bits 15..23
*/

static int LogicalToMSF ( int addr )
{
  return PhysToMSF (addr+150);
}

/* And this converts an MSF address to a Logical block address */
static int MSFtoLogical (int addr)
{
  return MSFtoPhys (addr-150);
}

/* ------------------------ */

/* This fn fills in a 4-byte logical or MSF address 
   it takes a pointer to the place to put it, the value in Logical format, 
   and a bool which is true for MSF format
*/
   
static void FillInAddress (void * ptr, int CD_addr, BOOL MSF_format)
{
  CHAR *p;
  
  p = ptr;
  if (MSF_format) CD_addr = LogicalToMSF(CD_addr);
  p[0] = BYTE4 (CD_addr);
  p[1] = BYTE3 (CD_addr);
  p[2] = BYTE2 (CD_addr);
  p[3] = BYTE1 (CD_addr);
}

/*******************************
*
* Diagnostic functions
*
*  CDHPCshowCDB prints out a SCSI command block
*/
VOID CDHPCShowCDB (PSCSI_REQUEST_BLOCK Srb)
{
  PUCHAR p;
               
  p=(UCHAR *) (Srb->Cdb);
  switch (Srb->CdbLength)
  {
    case 6:
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n", p[0], p[1], p[2], p[3], p[4], p[5] )); 
    break; 
    
    case 10:
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", p[0], p[1], p[2], p[3], p[4], p[5] )); 
    HPCDebugPrint(0x08,(0, "%x %x %x %x\n ", p[6], p[7], p[8], p[9] )); 
    break; 

    case 12:
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", p[0], p[1], p[2], p[3], p[4], p[5] )); 
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n", p[6], p[7], p[8], p[9], p[10], p[11] )); 
    break; 
    
    default:
    HPCDebugPrint(0x08,(0, "unexpected CDBLength:%x\n", Srb->CdbLength ));
  }
}

VOID CDHPCShowData (void * datapointer, int length)
{
  PUCHAR p;
  int i;
               
  p=(UCHAR *) (datapointer);
  HPCDebugPrint(0x08,(0, "Datalength:%x\n ",length ));  
  for (i=0; i<length; i+=6)
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5] ));  
}

VOID CDHPCShowDataText (void * datapointer, int length)
{
  PUCHAR p;
  int i;
               
  p=(UCHAR *) (datapointer);
  HPCDebugPrint(0x08,(0, "Datalength:%x\n ",length ));  
  for (i=0; i<length; i+=6)
  {
    HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5] ));  
    //HPCDebugPrint(0x08,(0, "%c %c %c %c %c %c\n ", p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5] ));
  }
}

/**** end of diagnostics ******/

#if CACHING
/*************************************************************************
*
*    InvalidateCache
*  
*   Sets the cache tags to 'not valid' e.g. because disc has changed
*/
VOID CDHPCInvalidateCache ( PSPECIFIC_LU_EXTENSION Lu)
{
  int i;
  for (i=0;i<CD_CACHE_SIZE;i++)
    Lu->CacheBlock[i] = BLOCK_NOT_VALID;
}
#endif

/*************************************************************************
*
*    DrvNumFn
*  
*   Generate Drive number from ID & Lun
*/
int DrvNumFn ( BYTE ID, BYTE LUN)
{
  // this seems to mess up with normal scsiport driver due to invalid buffers
  // return (ID * 8 + LUN); this seems to mess
  return (ID);
}

/*************************************************************************
*
*    SenseOK
*  
*   fills in SenseData block with 'everything OK' info.   
*/
VOID CDHPCSenseOK(IN SENSE_DATA * SD)
{
  SD->Valid=SETBITON;
  SD->ErrorCode = 0x70; //byte 0 - error for this command, not an old one - we aren't doing caching
  SD->SegmentNumber = 0; //byte 1 - only relevant for COPY,COMPARE,COPY&VERIFY
  SD->FileMark =SD->Reserved=SD->IncorrectLength=SD->EndOfMedia=SETBITOFF; //byte 2
  SD->SenseKey = SCSI_SENSE_NO_SENSE; //byte2
  SD->Information[0] = 0; //byte 3
  SD->Information[1] = 0;
  SD->Information[2] = 0;
  SD->Information[3] = 0;
  SD->AdditionalSenseLength = 10; //byte 7
  SD->CommandSpecificInformation[0] = 0;
  SD->CommandSpecificInformation[1] = 0;
  SD->CommandSpecificInformation[2] = 0;
  SD->CommandSpecificInformation[3] = 0;
  SD->AdditionalSenseCode = SCSI_ADSENSE_NO_SENSE; //byte 12
  SD->AdditionalSenseCodeQualifier = 0; //byte 13
  SD->FieldReplaceableUnitCode = 0;
  SD->SenseKeySpecific[0]= 0;
  SD->SenseKeySpecific[1]= 0;
  SD->SenseKeySpecific[2]= 0; //byte 17 - ie 18 bytes.
  
}

/***************************************************
*
*    DiskChanged
*
*  Stuff to re-get if the disk has been changed
*/

VOID CDHPCDiskChanged (PSPECIFIC_LU_EXTENSION Lu, PSCSI_REQUEST_BLOCK Srb)
{
  /* find out type of disk - data, audio, both etc */
  // for CDFS 2 we have to do this ourselves by getting track info and comparing data types
  // Use fake Lu and Srb as we don't want any real status changed by this access. 
  // We are 'snooping' here :-)
  
  SPECIFIC_DEVICE_EXTENSION fakeDev;
  SCSI_REQUEST_BLOCK fakeSrb;
  CHAR * TocDataBuffer ; //max possible length of track list is 804 bytes
  int i;
  
  fakeSrb.Cdb[6] = 0; //get whole list
  fakeSrb.Cdb[1] = 0; //don't use MSF format
  fakeSrb.TargetId = Srb->TargetId; /* point at correct target */
  fakeSrb.Lun = Srb->Lun;
  //fakeSrb.DataBuffer = TocDataBuffer; /* somwhere to put the data */
  fakeSrb.DataBuffer = Srb->DataBuffer; /* somwhere to put the data */

  
  // go and get the info
  CDHPCDoReadTOC (&fakeDev, &fakeSrb);
  
  //now check track data types and decide disk type (data/audio/both)
  TocDataBuffer = Srb->DataBuffer; //!! map pointers
  for (i=5; i<TocDataBuffer[1]; i+=8) //step though the ADR/Control entries in the track list
  {
    if (TocDataBuffer[i] & 0x40) // for each check the audio/data bit - set for data
      Lu->ModePages.header.MediumType &= 0x01; //set this bit for data tracks present
    else
      Lu->ModePages.header.MediumType &= 0x02; //set this bit for audio tracks present
  }
  
}




/*************************************************************************
*
*    SetReturnStatus
*  
*   Checks status and audiostatus returns from !PC. 
*   Fills in SRB and senseData accordingly.
*   SCSIOP (Srb->Cdb[0) is used to vary statusOK criteria
*   The length passed in is the length to return on success.

*   returns TRUE if status was OK, FALSE if it wasn't
*/
BOOL CDHPCSetReturnStatus (PSPECIFIC_LU_EXTENSION Lu, PSCSI_REQUEST_BLOCK Srb,
                              ULONG datalength)
{
  BOOL StatusNotOK;
  
  //forTestUnitReady and drivecapacity
  // if status is not OK, or drive is empty then return 'not ready' 

if (Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY || Srb->Cdb[0] == SCSIOP_READ_CAPACITY)
{
  StatusNotOK =  ( ((Lu->LastAccessStatus == CDERR_OK) && (Lu->DiskSize == 0))
                 || (Lu->LastAccessStatus != CDERR_OK)
                 || (Lu->LastAccessAudioStat != AUDIO_STOPPED) );               
}
// for other commands disk size does not get returned.
else
  StatusNotOK = (Lu->LastAccessStatus != CDERR_OK) || (Lu->LastAccessAudioStat != AUDIO_STOPPED);
  
  if (StatusNotOK)
  {
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->DataTransferLength = 0;
    
   #if CACHING 
    CDHPCInvalidateCache(Lu); //be paranoid for now- invalidate on any error
   #endif 
    /* needs extra sense info...eg block, length etc*/
    //what about BADFUNC?
    if (Lu->DiskSize == 0) 
    {
        Lu->SenseData.SenseKey = SCSI_SENSE_NOT_READY;
        Lu->SenseData.AdditionalSenseCode = SCSI_ADSENSE_LUN_NOT_READY;
    }
        //note if status has just gone from not ready to ready should set ASC 28, ASCQ 00 
    if (Lu->LastAccessStatus == CDERR_BADDRIVE) /* drive doesn't exist */
      Lu->SenseData.SenseKey = SCSI_SENSE_HARDWARE_ERROR;
    if (Lu->LastAccessStatus == CDERR_BADPARAMS) /* bad sector count */
      Lu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;  
    if (Lu->LastAccessStatus == CDERR_CHANGED)   /* medium changed */
      Lu->SenseData.SenseKey = SCSI_SENSE_UNIT_ATTENTION;
    if (Lu->LastAccessStatus == CDERR_BUSY)      /* busy  - many reasons*/
      Lu->SenseData.SenseKey = SCSI_SENSE_NOT_READY;
    if (Lu->LastAccessStatus == CDERR_READERR)   /* read failed - many reasons */
      Lu->SenseData.SenseKey = SCSI_SENSE_MEDIUM_ERROR;
    if (Lu->LastAccessStatus == CDERR_DRIVEFAIL) /* went wrong - who knows why */
      Lu->SenseData.SenseKey = SCSI_SENSE_HARDWARE_ERROR;
        
    if (Lu->LastAccessAudioStat != AUDIO_STOPPED)
    {
      Lu->SenseData.SenseKey = SCSI_SENSE_NOT_READY; 
      if (Lu->LastAccessAudioStat == AUDIO_PLAYING)
        Lu->SenseData.AdditionalSenseCode = 0x11;
      if (Lu->LastAccessAudioStat == AUDIO_PAUSED)
        Lu->SenseData.AdditionalSenseCode = 0x12;
    }
    return FALSE;
  }
  else //call was OK
  {
    Srb->ScsiStatus = SCSISTAT_GOOD;
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->DataTransferLength = datalength; //this value suplied by SCSIop function
    CDHPCSenseOK(&(Lu->SenseData));
    return TRUE;
  }  
}


/*************************************************************************
*
*    DoInquiry
*
*    returns a filled-in INQUIRY block
*    This routine is the only one that responds to non-existent LUNs
*/

#define SCSI1 0x01 //values for the Versions & responseformat fields
#define SCSI2 0x02

VOID 
CDHPCDoInquiry( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                IN OUT PSCSI_REQUEST_BLOCK     Srb
                )
{
  PINQUIRYDATA Iq;

  HPCDebugPrint(0x10,(0, " :CDHPCdoInquiry\n"));

  /*if (DrvNumFn(Srb->TargetId,Srb->Lun)() <= deviceExtension->NumCDDrives) //valid LUN - !!this is now wrong!
  { */ 
  Iq = Srb->DataBuffer;  //map inquiry struct over data buffer
  Iq->DeviceType = READ_ONLY_DIRECT_ACCESS_DEVICE; //CDROM
  Iq->DeviceTypeQualifier = 0; // 3 bits: connected to this LUN
  Iq->DeviceTypeModifier = 0; //5 bits: does nothing
  Iq->RemovableMedia = SETBITON;  //1 bit:removable
  Iq->Versions = Iq->ResponseDataFormat = SCSI2; //scsi 2 device 
  Iq->AdditionalLength = 31;    //length of rest of data
  memcpy (Iq->VendorId, "ALEPH1  ", 8);
  memcpy (Iq->ProductId, "HOSTED CDROM    ",16);
  memcpy (Iq->ProductRevisionLevel, "0.01",4);

  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  Srb->ScsiStatus = SCSISTAT_GOOD;
  Srb->DataTransferLength = INQUIRYDATABUFFERSIZE;  /*36*/
  CDHPCSenseOK(&deviceExtension->ActiveLu->SenseData);
  /*else  // not a valid Lun
  {
  Iq = Srb->DataBuffer;  //map inquiry struct over data buffer
  Iq->DeviceType = LOGICAL_UNIT_NOT_PRESENT_DEVICE; //0x7f, ie 0x03+-0x1f
  Iq->DeviceTypeQualifier = 0x03; // 3 bits: does not support a device for this lun
  Iq->DeviceTypeModifier = 0; //5 bits: does nothing
  Iq->RemovableMedia = SETBITOFF;  //1 bit:removable
  Iq->Versions = Iq->ResponseDataFormat = 0; //scsi 2 device - maybe 1 would be better
  Iq->AdditionalLength = 0;    //length of rest of data

  
  Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
  Srb->ScsiStatus = SCSISTAT_GOOD;
  Srb->DataTransferLength = 5;
  CDHPCSenseOK(&deviceExtension->ActiveLu->SenseData);
  }*/
  // this stuff is common to populated and non-populated Luns
  Iq->Reserved[0] = Iq->Reserved[1] = 0;
  Iq->SoftReset = 0;   //all these 8 are single-bit fields
  Iq->CommandQueue = 0;
  Iq->Reserved2 = 0;
  Iq->LinkedCommands = 0;
  Iq->Synchronous = 0;
  Iq->Wide16Bit = 0; //maybe we can support this one...?
  Iq->Wide32Bit = 0;
  Iq->RelativeAddressing = 0;

} //end DoInquiry

/*************************************************************************
*
*    DoRequestSense
*
*    Return errr info after 'check condition' scsi code
*    Sense data is updated by all commands, so whatever it there is right
*    All we do here is copy it to the Srb's databuffer, then reset it.
*/


VOID 
CDHPCDoRequestSense ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
  UINT lengthoftransfer;
  SENSE_DATA * psensedata;
  
  /* SCSI command 03 */

  HPCDebugPrint(0x10,(0, " :CDHPCdoRequestSense\n")); 
  
  psensedata = &(deviceExtension->ActiveLu->SenseData);
  // make sure we don't transmit more data than we have, or more than size of buffer
  
  lengthoftransfer = Srb->DataTransferLength;
  if (Srb->DataTransferLength > sizeof(deviceExtension->ActiveLu->SenseData)) 
      lengthoftransfer = sizeof(deviceExtension->ActiveLu->SenseData);
      
      
  memcpy (Srb->DataBuffer, psensedata, lengthoftransfer);
  
  HPCDebugPrint(0x08,(0, "length:%x SenseKey:%x, ASC:%x, QASC:%x\n", lengthoftransfer,
        psensedata->SenseKey, psensedata->AdditionalSenseCode, psensedata->AdditionalSenseCodeQualifier ));
        
  Srb->ScsiStatus = SCSISTAT_GOOD;
  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  Srb->DataTransferLength = lengthoftransfer;
  CDHPCSenseOK(psensedata);
  
} //end DoRequestSense

/*************************************************************************
*
*    DoPlayAudio 
*
*    Controls Audio play. 
*  takes SCSIOPS: PLAY_AUDIO (45), 
*/


VOID 
CDHPCDoPlayAudio    ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
  struct CD_PLAY_PARAMS   HPCCmdBuf;      /*data to send*/
  struct CD_PLAY_RESULT   HPCCDInfo;      /*status to get back*/
  HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/

  HPCDebugPrint(0x10,(0, " :CDHPCDoPlayAudio\n"));
  //Read data
  //set up command block -
  //set CDDISK_INFO command: 0300:0300
  
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_PLAY;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  HPCCmdBuf.sect_start= MKLONGCDB(2);
  HPCCmdBuf.play_len = MKWORDCDB(7);
  if (HPCCmdBuf.play_len!=0) //then play something, otherwise return 'OK'
  {
    HPCDebugPrint(0x08,(0, "\nPlay: for %x from block %x",HPCCmdBuf.play_len, HPCCmdBuf.sect_start));
                            
    HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
                 sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
                 sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
               
    HPCExecCall (deviceExtension, HPCCommandBlk);
  
    HPCDebugPrint(0x20,(0, "\nPlayAudioreturns: status=%x, audiostat=%x",
                            HPCCDInfo.status, HPCCDInfo.audiostat));
  
  }
  else
  {
    HPCCDInfo.status=CDERR_OK;
    HPCCDInfo.audiostat=AUDIO_STOPPED; //Is this OK, as we made it up?
  }
  deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
  deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
  
  CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, 0);
        
     //may also need to implement 'fallen off end of track'->cc, BLANK CHECK and
     //  END OF USER AREA ENCOUNTERED ON THIS TRACK
     //if start adress is not in a track, then give ->CC, BLANK CHECK and
     // ILLEGAL MODE FOR THIS TRACK
     
     //Also need to fill in LUExt->audiostatus with 'started', 'paused', 'none'etc.
        
   
//  For now tell them we don't support audio commands - 
//  - this didn't seem to stop it asking - mutter
 /*   Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->DataTransferLength = 0;
    deviceExtension->ActiveLu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;*/

  
  
} //end DoPlayAudio

/*************************************************************************
*
*    DoReadSubChannel
*
* 
*  takes SCSIOPS: Read SubChannel (42), 
*/


VOID 
CDHPCDoReadSubChannel ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                        IN OUT PSCSI_REQUEST_BLOCK     Srb
                      )
{
  
  struct CD_QCHANNEL_PARAMS   HPCCmdBuf;      /*data to send*/
  struct CD_QCHANNEL_RESULT   HPCCDInfo;      /*status to get back*/
  HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/
  int datalength, address;  /* to keep track of return data length, and do MSF->logical conversions */
  pSubQ_Data p;      /* pointer to struct for return SubQ data */

  HPCDebugPrint(0x10,(0, " :CDHPCDoReadSubChannel\n"));
  //Read data
  //set up command block -
  //set CDDISK_INFO command: 0300:0300
  
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_QCHANNEL;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  HPCDebugPrint(0x08,(0, "\nReadSubChannel"));
                          
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
             
  HPCExecCall (deviceExtension, HPCCommandBlk);

  HPCDebugPrint(0x20,(0, "\nReadSubChannel returns: status=%x, audiostat=%x",
                          HPCCDInfo.status, HPCCDInfo.audiostat));

  if (Srb->Cdb[2] & 0x40) /* return SubQ bit set */
  {
    /* assemble header in buffer*/
    p = Srb->DataBuffer;
    p->Reserved1 = 0;
    p->Audio_Status = deviceExtension->ActiveLu->AudioStatus;
    p->Data_Len[0] = 0;
    p->Data_Format = Srb->Cdb[3];
    p->Subq_Ctrl_ADR_rec.Control = HPCCDInfo.Qdata[0] & 0x0f;
    p->Subq_Ctrl_ADR_rec.ADR = ( HPCCDInfo.Qdata[0] & 0xf0 ) >> 4;
    p->Track = HPCCDInfo.Qdata[1];
    p->Index = HPCCDInfo.Qdata[2];
    address = MSFtoLogical (MKLONG( HPCCDInfo.Qdata[6], HPCCDInfo.Qdata[7], HPCCDInfo.Qdata[8], HPCCDInfo.Qdata[9]) );
    FillInAddress (&(p->AbsAddr[0]), address ,Srb->Cdb[1] & 0x02);
    address = MSFtoLogical (MKLONG( 0, HPCCDInfo.Qdata[3], HPCCDInfo.Qdata[4], HPCCDInfo.Qdata[5]) );
    FillInAddress (&(p->TrackRelAddr[0]), address  ,Srb->Cdb[1] & 0x02);
    
    /* then put approprate length & data in */
    switch (Srb->Cdb[3])
    {
      case CD_READ_ALL_SUBQ:      /*0x00*/
        p->Data_Len[1] = 44;
        break;
      
      case CD_READ_POSITION_SUBQ: /*0x01*/
        p->Data_Len[1] = 12;
        break;
        
      case CD_READ_UPC_CODE_SUBQ: /*0x02*/
        p->Data_Len[1] = 20;
        break;
        
      case CD_READ_ISRC_SUBQ:     /*0x03*/
        p->Data_Len[1] = 20;
        break;
      
      default:                     /*not supported */
        p->Data_Len[1] = 0;
    }
    datalength = p->Data_Len[1]+3;
  }
  else /* return no data, and status 'OK'*/
  {
    datalength = 0;
  }
  
 
  deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
  deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
  
  CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, datalength);
  
} /*DoSubChannel */

/********************************************
*
*  CDHPCFillModePages
*
*   Sets up mode pages of given LU to default values
*/

void
CDHPCFillModePages ( MODE_PARAM_PAGES *mp)
{
  //header (different for 10-bytes version)
  mp->header.ModeDataLength = 0; //this will be overwritten
  mp->header.MediumType = 0x01; //CDROM data only. needs changing for audio
  mp->header.DeviceSpecificParameter = 0; //DPO & FUA (caching control) bits not supported
  mp->header.BlockDescriptorLength = 0;
  //block descriptors
  mp->block.DensityCode = 1; //2048 bytes per sector - different for audio CD?
  mp->block.NumberOfBlocks[0]=0; //zero means 'all of it'
  mp->block.NumberOfBlocks[1]=0; 
  mp->block.NumberOfBlocks[2]=0; //should diskinfo fill it in here?
  mp->block.Reserved=0;
  mp->block.BlockLength[0]=0;
  mp->block.BlockLength[1]=0x08; //2048 fpr block length? its hard to tell from the spec
  mp->block.BlockLength[2]=0x00;
  //page 01
  mp->errorpg.PageCode = 0x01;
  mp->errorpg.Reserved = SETBITOFF;
  mp->errorpg.PageSavable = SETBITOFF; //not saveable
  mp->errorpg.PageLength = 6;
  mp->errorpg.ErrorRecoveryParam = 0x20; //Errored blocks will be transferred (TB bit) before CHECK_CONDITION reported
  mp->errorpg.ReadRetry = 0x02; // two retries will be attempted
  mp->errorpg.MERP_Reserved[0] = 0;
  mp->errorpg.MERP_Reserved[1] = 0;
  mp->errorpg.MERP_Reserved[2] = 0;
  mp->errorpg.MERP_Reserved[3] = 0;
  
  
  //page 02
  mp->disconnectpg.PageCode = 0x02;
  mp->disconnectpg.Reserved = SETBITOFF;
  mp->disconnectpg.PageSavable =SETBITOFF; //not saveable
  mp->disconnectpg.PageLength = 14;
  mp->disconnectpg.BufferFullRatio = 0;
  mp->disconnectpg.BufferEmptyRatio = 0;
  mp->disconnectpg.BusInactivityLimit[0] = 0;
  mp->disconnectpg.BusInactivityLimit[1] = 0;
  mp->disconnectpg.BusDisconnectTime[0] = 0;
  mp->disconnectpg.BusDisconnectTime[1] = 0;
  mp->disconnectpg.BusConnectTime[0] = 0;
  mp->disconnectpg.BusConnectTime[1] = 0;
  mp->disconnectpg.MaximumBurstSize[0] = 0;
  mp->disconnectpg.MaximumBurstSize[1] = 0;
  mp->disconnectpg.DataTransferDisconnect = 0;
  mp->disconnectpg.Reserved2[0] = 0;
  mp->disconnectpg.Reserved2[1] = 0;
  mp->disconnectpg.Reserved2[2] = 0;
}


/*************************************************************************
*
*    DoModeSense
*    SCSIOPS 1A (6-byte CDB)
*
*    gets status info from the drive. We make most of this up....
*/


VOID 
CDHPCDoModeSense    ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
  UCHAR p[256];             //buffer to assemble modepages in, before copying into DataBuffer
                            //we assemble everything in a buffer to be parnoid about exceptions from 
                            //falling off the end.
  UCHAR *n;                 //temp pointer for testing
  int datalength = 0;       //used to keep totalling length of stuff returned
  int pagesize;             //size of page about to be added to returned data
  BOOL DataTooBig = FALSE;  //true if we run out of space for returning mode pages
  int i;
  BYTE pcflags;            // 2bits: 00 = current, 01 = changeable, 10= default, 11=saved

  pcflags = (Srb->Cdb[2] & 0xC0) >> 6; //mask bits and shift to bottom
  HPCDebugPrint(0x10,(0, " :CDHPCdoModeSense - Page=%x, PC bits=%x\n", 
                Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK, pcflags) ); 

  //copy in mode sense parameter header 
  memcpy (p, &(deviceExtension->ActiveLu->ModePages.header), 
                              sizeof(deviceExtension->ActiveLu->ModePages.header));
  datalength = sizeof(deviceExtension->ActiveLu->ModePages.header); 
  p[0] = datalength -1; //length doesn't include itself
  
  // put in block descriptor(s) if bit set requesting them
  // if (Srb->Cdb[1] & 0x80) //block descriptors permitted. - not required though, so ignored
  
  p[3] = 0;  // set block desc length to 0
  datalength += 0;
  
  //put in requested page (if page 3f, we should do all of them...)
  switch (Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)
  {
    case 0x00:
    {
    if (pcflags == 0)
      // in this case send no data apart from the parameter block above (SCSI-1 compatibility)
      break;
    else
      //page 00 is not supported by us
      goto unsupported;
    }
        
    case 0x01: //Read/write error page
    {
      // we need to make sure that modepage data is uptodate - 
      // !!until the 'diskchanged' Fn is run from somewhere more sensible!!
      //CDHPCDiskChanged(deviceExtension->ActiveLu, Srb);
      
      pagesize = sizeof(deviceExtension->ActiveLu->ModePages.errorpg);
      if ((pagesize+datalength) > Srb->DataTransferLength)
      {
        DataTooBig = TRUE;
        break;
      }
      else
      {
        //copy the data into place, 
        n=(CHAR *) &(deviceExtension->ActiveLu->ModePages.errorpg);
        memcpy (p + datalength, &(deviceExtension->ActiveLu->ModePages.errorpg), pagesize);
        if (pcflags == 1) //return all ones for 'changeable' values, all zeros for nonchangeable ones
        {
          deviceExtension->ActiveLu->ModePages.errorpg.ErrorRecoveryParam = 0;
          deviceExtension->ActiveLu->ModePages.errorpg.ReadRetry = 0;
        }
        //add length to tally
        datalength += pagesize;
        HPCDebugPrint(0x08,(0, " Page 01:pagesize=%x, Datalength=%x and contents\n ", pagesize, datalength )); 
   /*     for (i=0; i < pagesize; i+=6)
          HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", n[i+0], n[i+1], n[i+2], n[i+3], n[i+4], n[i+5] ));*/
        break;
      }
    }
                        
    
    case 0x02: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x03: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x07: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x08: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x09: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x0A: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x0B: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x0D: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x0E: //disconnect-reconnect page
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    case 0x3f: //need to return all the pages
    HPCDebugPrint(0x10,(0, "Page =%x requested - implement it!!\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
    break;
    
    unsupported:
    default:  //unrecognised page.
              
    {
      HPCDebugPrint(0x10,(0, "Page =%x not supported\n ", Srb->Cdb[2] & SCSI_MODE_SENSE_PAGE_MASK)); 
      Srb->SrbStatus = SRB_STATUS_ERROR;
      Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
      Srb->DataTransferLength = 0;
        deviceExtension->ActiveLu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
        deviceExtension->ActiveLu->SenseData.AdditionalSenseCode = SCSI_ADSENSE_BAD_CDB;
    } 
  }
  

  // pages now copied into databuffer. Check for overflow
  // (data doesn't fit in datalength specified (256 bytes for SCSIOP1A))
  
  if (DataTooBig)
  {
    HPCDebugPrint(0x10,(0, "DataBuffer (length=%x) overflowed\n ", Srb->DataTransferLength)); 
    p[0] = 0;
    
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    Srb->DataTransferLength = datalength;
    deviceExtension->ActiveLu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    deviceExtension->ActiveLu->SenseData.AdditionalSenseCode = SCSI_ADSENSE_BAD_CDB;
  }
  else //all OK, set header length to total length of header+blocks+pages, and retun 'OK'
  {
    HPCDebugPrint(0x08,(0, "Block completed: Datalength=%x\n ", datalength )); 
    p[0] = datalength-1; //length doesn't include itself
    for (i=0; i < datalength; i+=6)
      HPCDebugPrint(0x08,(0, "%x %x %x %x %x %x\n ", p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5] ));  
    /* now copy buffer into Srb */  
    memcpy (p, Srb->DataBuffer, Srb->DataTransferLength);           
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
    Srb->DataTransferLength = datalength;
    CDHPCSenseOK(&deviceExtension->ActiveLu->SenseData);
  } 
   

} //end DoModeSense


/*************************************************************************
*
*    DoCDControl
*
*    controls drive locking, ejecting etc
* takes SCIOPs 0x1E (medium removal control), 0x1B (load/unload)
*/

VOID 
CDHPCDoCDControl     ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb )
{
  struct CD_CONTROL_PARAMS   HPCCmdBuf;      /*data to send*/
  struct CD_CONTROL_RESULT   HPCCDInfo;      /*status to get back*/
  HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/

  HPCDebugPrint(0x10,(0, " :CDHPCdoCDControl "));

  //set up command block - fn code set to FFFF. It will be changed if an actual HPCControl call is needed.
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_CONTROL;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  HPCCmdBuf.fncode = CONTROL_NOACTION; //do no HPC action - gets changed below if needed
  
  // Assume all OK - gets changed later if call fails
  Srb->SrbStatus = SRB_STATUS_SUCCESS;
  Srb->ScsiStatus = SCSISTAT_GOOD;
  Srb->DataTransferLength = 0;
  CDHPCSenseOK(&deviceExtension->ActiveLu->SenseData);
  
  // now set up HPC Control command or flags as/if reuqired.
  switch  (Srb->Cdb[0])
  {
    // note that Cdb[1] bit 0 is 'immed' bit. If set should return status immediately, rather than after command
    // not implemented here.
    case SCSIOP_LOAD_UNLOAD: //0x1B
    {
      HPCDebugPrint(0x10,(0, "LOAD/UNLOAD\n"));
      if (!(deviceExtension->ActiveLu->ControlStatus & PREVENT_REMOVAL)) //drive is unlocked
      {
        switch (Srb->Cdb[4] & 0x03) //bit 0 is start(stop), bit 1 is eject(load) 
        {
        case 0x02:  //eject 
          HPCCmdBuf.fncode = CONTROL_OPEN;
          break;
        case 0x03:  //load
          HPCCmdBuf.fncode = CONTROL_CLOSE;
          break;
          
        case 0x00:  //stop device - make inaccessible
          deviceExtension->ActiveLu->ControlStatus |= PREVENT_ACCESS;
          break;
        case 0x01:  //start device - make accessible
          deviceExtension->ActiveLu->ControlStatus &= ~PREVENT_ACCESS;
          break;
        }
      }
      break;
    }
    
    //if prevent bit (bit 0 of CDB byte 4) is set then media cannot be removed. 
    // Strictly there is one lock per initiator, but I don't think we're going to get two (aspi driver?)
    
    case SCSIOP_MEDIUM_REMOVAL: //0x1E
    {
      HPCDebugPrint(0x10,(0, "MEDIUM REMOVAL\n"));
      //set status bit
      if (Srb->Cdb[4] & 0x01)
        HPCCmdBuf.fncode = CONTROL_LOCK;
      else
        HPCCmdBuf.fncode = CONTROL_UNLOCK;
      break;
    }
  }
  
  // if an action has been set then do HPC call, otherwise just return 'OK'
  if (HPCCmdBuf.fncode != CONTROL_NOACTION) //an action has been set
  {
    HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
    HPCExecCall (deviceExtension, HPCCommandBlk);
    
    HPCDebugPrint(0x08,(0, "CDControl: status=%x, audiostat=%x\n",
                            HPCCDInfo.status, HPCCDInfo.audiostat));
    
    deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
    deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;                
 
    if ( CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, 0) )
     {
       // all OK - set flags to indicate change has occurred
       if (HPCCmdBuf.fncode == CONTROL_UNLOCK)
         deviceExtension->ActiveLu->ControlStatus &= ~PREVENT_REMOVAL;
       if (HPCCmdBuf.fncode == CONTROL_LOCK)
         deviceExtension->ActiveLu->ControlStatus |= PREVENT_REMOVAL;
     }
                         
  } 
  //if no HPC action needed
  // Srb status has already defaulted to OK.
    
}   

/*************************************************************************
*
*    DoRead
*
*    reads track table of contents. SCSIOP 0x43
*/

VOID 
CDHPCDoReadTOC      ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb )
{
  struct CD_TRACKINFO_PARAMS   HPCCmdBuf;      /*data to send*/
  struct CD_TRACKINFO_RESULT   HPCCDInfo;      /*status to get back*/
  HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/
  unsigned int i, lengthrequested, datalength, starttrack;
  TOC_HEADER *ptoch;
  TOC_TRACK_DESCRIPTOR *ptrack;
  ULONG MSFstart = 0;

  HPCDebugPrint(0x10,(0, " :CDHPCdoReadTOC\n"));

  HPCDebugPrint(0x08,(0, "Length requested:%x, Cdblength:%x\n",
                     Srb->DataTransferLength,((Srb->Cdb[7])<<8)+(Srb->Cdb[8])));
  
  
  //Read data
  //set up command block -
  //set CDDISK_TRACKINFO command: 03 00:06 00
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_TRACKINFO;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  if (Srb->Cdb[6]==0)  HPCCmdBuf.trknum = 0xFFFF;  //do whole list
  else                 HPCCmdBuf.trknum = Srb->Cdb[6]; //start at this track - (or just do one track - spec isn't clear?)
  
  HPCDebugPrint(0x08,(0, "ReadTOC: from track %x, MSFflag=%x\n", HPCCmdBuf.trknum, Srb->Cdb[1] & 0x02));
  
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
  HPCExecCall (deviceExtension, HPCCommandBlk);
  
  HPCDebugPrint(0x08,(0, "1st track:%x, last track:%x\n", HPCCDInfo.firsttrk, HPCCDInfo.lasttrk));

  if (Srb->Cdb[6] > HPCCDInfo.lasttrk) //invalid track number - barf
  {
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    Srb->SrbStatus = SRB_STATUS_ERROR;
    deviceExtension->ActiveLu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    deviceExtension->ActiveLu->SenseData.AdditionalSenseCode = SCSI_ADSENSE_BAD_CDB;
  }
  else //carry on with command
  {
    if (Srb->Cdb[6]==0) // do whole TOC
    {
      starttrack = HPCCDInfo.firsttrk;
    }
    else //start at give track number
    {
      starttrack = Srb->Cdb[6];
    }  
    
    //set up TOC header (data actually going into databuffer)
    ptoch = (TOC_HEADER *) Srb->DataBuffer;
    // set length of return block - length bytes themselves don't count
    datalength = 2 + (HPCCDInfo.lasttrk - starttrack + 2) * sizeof(TOC_TRACK_DESCRIPTOR);
    ptoch->DataLength[0] = (UCHAR) ((datalength >> 8) & 0xFF);
    ptoch->DataLength[1] = (UCHAR) (datalength & 0xFF);
    ptoch->FirstTrack = HPCCDInfo.firsttrk; //is this first existing track, or first in this table?
    ptoch->LastTrack = HPCCDInfo.lasttrk;
    
    // get track info for each track from first requested track to last track
    for (i=starttrack; i<=ptoch->LastTrack; i++) //then call for each track
    {
      HPCCmdBuf.trknum = i;
      HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
                   sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
                   sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
      HPCExecCall (deviceExtension, HPCCommandBlk);
      
      //copy results into TOCdata...
      //first point each descriptor into relevant bit of databuffer. 4 bytes in header, then 8 for each track descriptor
      ptrack = (TOC_TRACK_DESCRIPTOR *) ( (CHAR *) (Srb->DataBuffer) + 4 + (i - starttrack) * sizeof(TOC_TRACK_DESCRIPTOR) );
      ptrack->Reserved1 = 0;
      ptrack->toc_ctrl_rec.ADR = 1; //Q-channel info encodes current poition - could mod HPC to get actual value?
      /*if (HPCCDInfo.trkflags & 0x40) HPCCDInfo.trkflags |= 0x20; //if data track, set copyable bit? */
      ptrack->toc_ctrl_rec.Control = HPCCDInfo.trkflags >> 4;
      ptrack->TrackID = i;
      ptrack->ReservedForHell2 = 0;
      
      // if MSFbit set in CDB we should return these in MSF format
      FillInAddress (&(ptrack->StartAddr[0]), HPCCDInfo.trkstart, Srb->Cdb[1] & 0x02);
      
      if (Srb->Cdb[1] & 0x02) MSFstart=LogicalToMSF(HPCCDInfo.trkstart);
      HPCDebugPrint(0x08,(0, "trackID:%x, flags:%x start:%x, MSF%x:\n", i, HPCCDInfo.trkflags, HPCCDInfo.trkstart,MSFstart));
          
    }
    //after all the tracks we need the lead-out area - note.. assumes one data track...
    ptrack = (TOC_TRACK_DESCRIPTOR *) ( (CHAR *) (Srb->DataBuffer) + 4 + (i - starttrack) * sizeof(TOC_TRACK_DESCRIPTOR) );
    ptrack->Reserved1 = 0;
    ptrack->toc_ctrl_rec.ADR = 1; //Q-channel info encodes current poition - could mod HPC to get actual value?
    /*if (HPCCDInfo.trkflags & 0x40) HPCCDInfo.trkflags |= 0x20; //if data track, set copyable bit? */
    ptrack->toc_ctrl_rec.Control = HPCCDInfo.trkflags >> 4;
    ptrack->TrackID = 0xAA;
    ptrack->ReservedForHell2 = 0;
    if (Srb->Cdb[1] & 0x02)
    { /* return MSF format */
      MSFstart=LogicalToMSF(deviceExtension->ActiveLu->DiskSize);
      ptrack->StartAddr[0] = BYTE4 (MSFstart);
      ptrack->StartAddr[1] = BYTE3 (MSFstart);
      ptrack->StartAddr[2] = BYTE2 (MSFstart);
      ptrack->StartAddr[3] = BYTE1 (MSFstart);
    }
    else
    { /* return Logical Block format */
      ptrack->StartAddr[0] = BYTE4 (deviceExtension->ActiveLu->DiskSize);
      ptrack->StartAddr[1] = BYTE3 (deviceExtension->ActiveLu->DiskSize);
      ptrack->StartAddr[2] = BYTE2 (deviceExtension->ActiveLu->DiskSize);
      ptrack->StartAddr[3] = BYTE1 (deviceExtension->ActiveLu->DiskSize);
    }

    
    HPCDebugPrint(0x08,(0, "CDreadTOC: status=%x, audiostat=%x\n",
                            HPCCDInfo.status, HPCCDInfo.audiostat));
    
    deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
    deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
    CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, datalength+2);                     

  }
  
} //end DoReadTOC

#if CACHEING
/*************************************************************************
*
*    DoRead
*
*    returns data if cached, otherwise reads data into buffer & cache.
*/

VOID 
CDHPCDoRead         ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
  int i;
  BOOL CacheHit = FALSE;
  
  HPCDebugPrint(0x10,(0, " :CDHPCdoRead\n"));
  
  HPCDebugPrint(0x08,(0, "Read: %x blocks from block %x\n",MKWORDCDB(7), MKLONGCDB(2) ));
  //check cache first - actually we can improve this if cache is always n consecutive blocks
  for (i=0; (i<CD_CACHE_SIZE) && (CacheHit==FALSE) ;i++)
  {
    if (deviceExtension->ActiveLu->CacheBlock[i] == MKLONGCDB(2))
    {
      CacheHit=TRUE;
      HPCDebugPrint(0x08,(0, "Cache hit: block %x, at tag %x\n", (MKLONGCDB(2)), i ));
      HPCDebugPrint(0x08,(0, "Cache content:%x %x %x %x ....", deviceExtension->ActiveLu->CacheBlock[0],deviceExtension->ActiveLu->CacheBlock[1], deviceExtension->ActiveLu->CacheBlock[2],deviceExtension->ActiveLu->CacheBlock[3]));
    }
  }
  
  if (CacheHit) //we're assuming single block reads here still
  { //why is i always one too large? oh well, fixed now.
    HPCDebugPrint(0x08,(0, "Cached block copied, from tag %x\n", i-1 ));
    memcpy(Srb->DataBuffer, &(deviceExtension->ActiveLu->CacheData[(i-1)*CD_BLOCK_SIZE]), CD_BLOCK_SIZE);
    Srb->ScsiStatus = SCSISTAT_GOOD;
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->DataTransferLength = CD_BLOCK_SIZE;
    CDHPCSenseOK(&deviceExtension->ActiveLu->SenseData);
  }
  else
  {
    CDHPCDoRealRead(deviceExtension, Srb);
  }
}
#endif
  
/*************************************************************************
*
*    DoRealRead
*
*    reads data into buffer
*/
  
VOID 
CDHPCDoRealRead     ( IN PSPECIFIC_DEVICE_EXTENSION  deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
  struct CD_READ_PARAMS   HPCCmdBuf;      /*data to send*/
  struct CD_READ_RESULT   HPCCDInfo;      /*status to get back*/
  HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/  //Read data
  int i;
  
  //set up command block -
  //set CDDISK_INFO command: 0300:0100
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_READ;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
#if CACHING  
  HPCCmdBuf.sect_count = CD_CACHE_SIZE; //should be cachesize, but only 1st 4K works - ie not quite two blocks! 
#else
  HPCCmdBuf.sect_count = MKWORDCDB(7); //we always get 1 (only 1st 4K works!) and return 1
#endif  
  HPCCmdBuf.sect_start = MKLONGCDB(2);
  HPCCmdBuf.readahead = 1;
  HPCDebugPrint(0x08,(0, "Cache miss - Read: %x blocks from block %x\n",HPCCmdBuf.sect_count, HPCCmdBuf.sect_start));
  if ((Srb->DataTransferLength >= CD_MAX_RD_LEN) || (HPCCmdBuf.sect_count > CD_READ_MAXSECTS))
  { // truncate read to prevent buffer overflow.
    HPCDebugPrint(0x08,(0, "Transfer length problem! Buffer size:%x, sector count:%x\n",
                            Srb->DataTransferLength, HPCCmdBuf.sect_count));
    HPCCmdBuf.sect_count = CD_READ_MAXSECTS;
    Srb->DataTransferLength = CD_READ_MAXSECTS * CD_BLOCK_SIZE;
  }

#if CACHING               
  //cached version
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, (WORD) (HPCCmdBuf.sect_count * CD_BLOCK_SIZE), &(deviceExtension->ActiveLu->CacheData[0]));
#else               
  //uncached version
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, (WORD) (HPCCmdBuf.sect_count * CD_BLOCK_SIZE), Srb->DataBuffer);
#endif               
  
  HPCExecCall (deviceExtension, HPCCommandBlk);
  
  HPCDebugPrint(0x08,(0, "\nRead returns: status=%x, audiostat=%x\n",
                          HPCCDInfo.status, HPCCDInfo.audiostat));
  
#if CACHING               
  /*CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[0]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[1*CD_BLOCK_SIZE]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[2*CD_BLOCK_SIZE]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[3*CD_BLOCK_SIZE]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[4*CD_BLOCK_SIZE]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[5*CD_BLOCK_SIZE]), 16);
  CDHPCShowDataText(&(deviceExtension->ActiveLu->CacheData[6*CD_BLOCK_SIZE]), 16);*/
#endif               
  
  deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
  deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
                       
  // if status is not OK, barf
  if ( CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, CD_BLOCK_SIZE) )
  {
    #if CACHEING
    //retrieve the requested block from the cache
    memcpy (Srb->DataBuffer, &(deviceExtension->ActiveLu->CacheData[0]), CD_BLOCK_SIZE);
    //and mark cached block numbers
    for (i=0;i<HPCCmdBuf.sect_count;i++)
      deviceExtension->ActiveLu->CacheBlock[i] = HPCCmdBuf.sect_start + i;
    #endif
  }
  else //everything is OK
  {
   #if CACHING 
    CDHPCInvalidateCache(deviceExtension->ActiveLu); //be paranoid for now - invalidate on any error
   #endif
  }
  
} //end DoRealRead

/*************************************************************************
*
*    DoReadCapacity - SCSIop 25
*
*    get capcity of drive - for HDs not CDs at this moment....
*/


VOID 
CDHPCDoReadCapacity ( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
   struct CD_DISKINFO_PARAMS   HPCCmdBuf;      /*data to send*/
   struct CD_DISKINFO_RESULT   HPCCDInfo;      /*data to get back*/
   HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/
   char * p;
  

  HPCDebugPrint(0x10,(0, " :CDHPCdoReadCapacity. PMI bit=%x\n", (Srb->Cdb[8] & 0x01)));
  // check CDROM status
  //set up command block - 
  //set CDDISK_INFO command: 0300:0500
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_DISKINFO;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
  
  HPCExecCall (deviceExtension, HPCCommandBlk);
  
  HPCDebugPrint(0x08,(0, "Diskinfo returns: status=%x, audiostat=%x, disksize=%x\n",
                          HPCCDInfo.status, HPCCDInfo.audiostat, HPCCDInfo.disksize));
                          
  deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
  deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
  deviceExtension->ActiveLu->DiskSize = HPCCDInfo.disksize;

  
  // if status is OK, with valid disk size, then fill in return block 
  if ( CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, 8) )  
  {
    // fill in return block, in logical block format
    //last block of drive
    FillInAddress (Srb->DataBuffer, HPCCDInfo.disksize-1, FALSE);
    //block size - 2048
    FillInAddress (Srb->DataBuffer,CD_BLOCK_SIZE , FALSE);
  } 
  
} //end DoReadCapacity

/*************************************************************************
*
*    DoTestUnitReady
*
*    returns ready status. Unit is ready when media is in a removable drive,
*    and it is not busy
*/


VOID 
CDHPCDoTestUnitReady( IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                      IN OUT PSCSI_REQUEST_BLOCK     Srb
                    )
{
   struct CD_DISKINFO_PARAMS   HPCCmdBuf;      /*data to send*/
   struct CD_DISKINFO_RESULT   HPCCDInfo;      /*data to get back*/
   HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/
  

  HPCDebugPrint(0x10,(0, " :CDHPCdoTestUnitReady\n"));
  // check CDROM status
  //set up command block - 
  //set CDDISK_INFO command: 0300:0500
  HPCCmdBuf.hpc_id = HPC_CD_ID;
  HPCCmdBuf.reason = HPC_CD_DISKINFO;
  HPCCmdBuf.drvnum = DrvNumFn(Srb->TargetId,Srb->Lun);
  
  HPCCmdSetup (&HPCCommandBlk, INTERRUPTFLG, HPC_BUFFER_B, 
               sizeof(HPCCmdBuf), &HPCCmdBuf, 0, NULL, 
               sizeof(HPCCDInfo), &HPCCDInfo, 0, NULL);
  
  HPCExecCall (deviceExtension, HPCCommandBlk);
  
  HPCDebugPrint(0x08,(0, "Diskinfo returns: status=%x, audiostat=%x, disksize=%x\n",
                          HPCCDInfo.status, HPCCDInfo.audiostat, HPCCDInfo.disksize));
                          
  deviceExtension->ActiveLu->LastAccessStatus = HPCCDInfo.status;
  deviceExtension->ActiveLu->LastAccessAudioStat = HPCCDInfo.audiostat;
  deviceExtension->ActiveLu->DiskSize = HPCCDInfo.disksize;

  CDHPCSetReturnStatus(deviceExtension->ActiveLu, Srb, 0);
  
  
} //end DoTestUnitReady


/*************************************************************************
*
*    CDHPCNotImplemented
*
*    Returns 'invalid request' for now - ie not supported by this driver
*/
CDHPCNotImplemented(   
           IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
           IN OUT PSCSI_REQUEST_BLOCK        Srb
           )
{
  Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
  Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
  deviceExtension->ActiveLu->SenseData.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;  
  
  HPCDebugPrint(0x10,(0, "Not Implemented!!\n"));
 
}//end NotImplemented


/*************************************************************************
*
*    CDHPCDoIO
*
*    Map the SCSI Cdb data into HPC format and then do the HPC call, returning the results
*
*
*/
CDHPCdoIO(   
           IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
           IN PSCSI_REQUEST_BLOCK        Srb
           )
{
  
  HPCDebugPrint(0x10,(0, "CDHPCdoIO: CDB="));
  CDHPCShowCDB (Srb);
  
  switch (Srb->Cdb[0])
  {
    
    //mandatory commands
    case SCSIOP_TEST_UNIT_READY:      /* 00 */
      CDHPCDoTestUnitReady (deviceExtension, Srb);
      break;
    case SCSIOP_REQUEST_SENSE:        /* 03 */
      CDHPCDoRequestSense (deviceExtension, Srb);
      break;
    case SCSIOP_INQUIRY:              /* 12 */
      CDHPCDoInquiry (deviceExtension, Srb);
      break;
    case SCSIOP_MODE_SELECT:          /* 15 */
      CDHPCNotImplemented (deviceExtension, Srb);
      break;
    case SCSIOP_RESERVE_UNIT:         /* 16 */
      CDHPCNotImplemented (deviceExtension, Srb);
      break;
    case SCSIOP_RELEASE_UNIT:         /* 17 */
      CDHPCNotImplemented (deviceExtension, Srb);
      break;
    case SCSIOP_MODE_SENSE:           /* 1A */
      CDHPCDoModeSense (deviceExtension, Srb);
      break;
    case SCSIOP_SEND_DIAGNOSTIC:      /* 1D */
      CDHPCNotImplemented (deviceExtension, Srb);
      break;
    case SCSIOP_MEDIUM_REMOVAL:       /* 1E */
    case SCSIOP_LOAD_UNLOAD:          /* 1B */
      CDHPCDoCDControl (deviceExtension, Srb);
      break;
    case SCSIOP_READ_CAPACITY:        /* 25 */
      CDHPCDoReadCapacity (deviceExtension, Srb);
      break;
    case SCSIOP_READ:                 /* 28 */
    #if CACHING
      CDHPCDoRead (deviceExtension, Srb);
    #else
      CDHPCDoRealRead (deviceExtension, Srb);
    #endif  
      break;
    case SCSIOP_SEEK:                 /* 2B */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_SEEK
      break;
    case SCSIOP_READ_SUB_CHANNEL:     /* 42 */
      CDHPCDoReadSubChannel (deviceExtension, Srb);
      break;
    case SCSIOP_READ_TOC:             /* 43 */
      CDHPCDoReadTOC (deviceExtension, Srb);
      break;
    //audio commands
    case SCSIOP_PLAY_AUDIO_10:        /* 45 */
      CDHPCDoPlayAudio (deviceExtension, Srb);
      break;
    case SCSIOP_PLAY_AUDIO_MSF:       /* 47 */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_PLAY
      break;
    case SCSIOP_PLAY_TRACK_INDEX:     /* 48 */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_PLAY
      break;
    case SCSIOP_PLAY_TRACK_RELATIVE:  /* 49 */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_PLAY
      break;
    case SCSIOP_PAUSE_RESUME:         /* 4B */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_CONTROL
      break;
    case SCSIOP_PLAY_AUDIO:           /* A5 */
      CDHPCNotImplemented (deviceExtension, Srb);
      //HPC_CD_PLAY
      break;
   
       
    default:
    ;// unsupported op
    CDHPCNotImplemented (deviceExtension, Srb);

    
  }
  if (Srb->Cdb[0] != SCSIOP_READ) //don't bother for read data - there's too much of it
    CDHPCShowData(Srb->DataBuffer, Srb->DataTransferLength);
  else
    CDHPCShowDataText(Srb->DataBuffer, 40); // show 1st 40 bytes for reads
    
}           
           


/*************************************************************************
* Routine Name:
*
*    CDHPCInterrupt
*
* Routine Description:
* 
*    This routine handles the interrupts for the CDHPC.  it currently does
*    nothing. It could complete HPC transfers...
* 
* Arguments:
* 
*    Context - Device adapter context pointer.
* 
* Return Value:
* 
*    TRUE indicates that the interrupt was from this CDHPC adapter,
*    FALSE indicates that this interrupt was NOT from us.
* 
*/
BOOLEAN
CDHPCInterrupt(
   IN PVOID Context
   )
{
//   PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;

   HPCDebugPrint(0x10,
                 (0, "\nCDHPCInterrupt ignored: Dev= %x\n ",
                 Context));
}

/*************************************************************************
* Routine Name:
*
*    CDHPCStartExecution
*
* Routine Description:
* 
*    This routine will start the execution of a SCSI request.
* 
* Arguments:
* 
*    DeviceExtension -   Device adapter context pointer.
*    LuExtension -       The logical unit specific information.
*    Srb -               The Srb command to execute.
* 
* Return Value:
* 
*    None.
* 
*/
VOID
CDHPCStartExecution(
   IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
   IN PSPECIFIC_LU_EXTENSION     LuExtension,
   IN PSCSI_REQUEST_BLOCK        Srb
   )
{
   HPCDebugPrint(0x10,
                 (0, "CDHPCStrtEx Dev= %x, LuExt= %x, Srb= %x ",
                 DeviceExtension,
                 LuExtension,
                 Srb));
   HPCDebugPrint(0x10,
                 (0, "ID= %x, Lun= %x\n",
                 Srb->TargetId,
                 Srb->Lun));

   // return SRB_STATUS_NO_DEVICE if target doesn't exist.
   // IDentify wll deal with this if Lun doesn't exist.
                 
   if ((Srb->TargetId >= (int) (DeviceExtension->NumCDDrives)) || (Srb->Lun != 0))
   {
     Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
     if (Srb->TargetId == SCSI_INITIATOR_ID) //can't talk to host ccontroller!
       Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
     Srb->DataTransferLength = 0;
     ScsiPortNotification(RequestComplete,
                          (PVOID) DeviceExtension,
                          Srb);
     ScsiPortNotification(NextRequest,
                          (PVOID) DeviceExtension,
                          NULL);
   }
   else
   {
     //
     // Setup the context for this adapter.
     //
     DeviceExtension->ActiveLu = LuExtension;
   
     // Check for Bus Free
     /*HPCDebugPrint(0x10,
                   (0, "HPCstatus= %x\n",DeviceExtension->HPCstatus));*/

     if (DeviceExtension->HPCstatus & HPCST_READY)
     {
       CDHPCdoIO(DeviceExtension, Srb);

       HPCDebugPrint(0x10,(0, " SRB status= %x SCSI Status= %x\n\n",Srb->SrbStatus, Srb->ScsiStatus));

       ScsiPortNotification(RequestComplete,
                            (PVOID) DeviceExtension,
                            Srb);
       ScsiPortNotification(NextRequest,
                            (PVOID) DeviceExtension,
                            NULL);

     }
   }
  
return;

} // end CDHPCStartExecution()



/*************************************************************************
* Routine Name:
*
*    CDHPCAbort
*
* Routine Description:
* 
*    Attempt to abort a command on a SCSI target. Since there is no way to
*    abort a command on the CDHPC,  this routine can only
*    return with an error.
* 
* Arguments:
* 
*    DeviceExtension -   The adapter specific information.
*    LuExtension     -   The specific target/logical unit information.
*    Srb -               The Srb command to execute.
* 
* Return Value:
* 
*     None.
* 
*/
VOID
CDHPCAbort(
   IN PSPECIFIC_DEVICE_EXTENSION DeviceExtension,
   IN PSPECIFIC_LU_EXTENSION     LuExtension,
   IN PSCSI_REQUEST_BLOCK        Srb
   )
{
   PUCHAR              baseAddress = DeviceExtension->BaseAddress;
   PSCSI_REQUEST_BLOCK srbBeingAborted = LuExtension->ActiveLuRequest;

   HPCDebugPrint(0x10, (0, "CDHPCAbort: failed.\n"));

   Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;


} // end CDHPCAbort()


/*************************************************************************
* Routine Name:
*
*    CDHPCResetBus
*
* Routine Description:
* 
*    Reset CDHPC SCSI adapter (no action for this) 
*      maybe we should reset mode pages info?
*    and SCSI bus.
* 
* Arguments:
* 
*    Context for the reset.
*    PathId
* 
* Return Value:
* 
*    Nothing.
* 
*/
BOOLEAN
CDHPCResetBus(
   IN PVOID Context,
   IN ULONG PathId
   )
{
   PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;

   HPCDebugPrint(0x800, (0, "CDHPCResetBus: Reset CDHPC\n"));

   deviceExtension->InterruptPending = FALSE;

   //
   // RESET HPC.
   //
   // set SCSI config info back to defaults?
   //CDHPCFillModePages (deviceExtension->ActiveLu->ModePages); 
   
   //
   // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
   //


   ScsiPortCompleteRequest(deviceExtension,
                           (UCHAR) PathId,
                           (UCHAR) -1,
                           (UCHAR) -1,
                           SRB_STATUS_BUS_RESET);

   ScsiPortNotification(NextRequest,
                           deviceExtension,
                           NULL);

   return TRUE;
} // end CDHPCResetBus()


/*************************************************************************
* Routine Name:
*
*    CDHPCStartIo
*
* Routine Description:
* 
*    This routine is called from the SCSI port driver synchronized
*    with the kernel with a request to be executed. It also initialises
*    'target' info, such as Mode Sense Tables. 
*    
* Arguments:
*    Context -   The adapter specific information.
*    Srb -       The Srb command to execute.
* 
* Return Value:
* 
*    TRUE
* 
*/
BOOLEAN
CDHPCStartIo(
   IN PVOID               Context,
   IN PSCSI_REQUEST_BLOCK Srb
   )
{
   PSPECIFIC_DEVICE_EXTENSION  deviceExtension = Context;
   PSPECIFIC_LU_EXTENSION      luExtension;

   HPCDebugPrint(0x20,
                 (0, "CDHPCStartIo: PathID=%x, Dev= %x, Srb= %x, ",
                 Srb->PathId, deviceExtension, Srb ));

   //
   // Determine the logical unit that this request is for.
   //
   deviceExtension->PathId = Srb->PathId;
   luExtension = ScsiPortGetLogicalUnit(deviceExtension,
                                        deviceExtension->PathId,
                                        Srb->TargetId,
                                        Srb->Lun);
                                        
   if (luExtension == NULL) //we should bale out if this happens...but it never seems to
     HPCDebugPrint(0x20, (0, "luExtension pointer is NULL - ERK!\n" ));
   if (luExtension->LuState == LS_NOTINITED)
   {  
     //initialize it. Set up deafult MODE SENSE pages.
     CDHPCFillModePages (&(luExtension->ModePages));
     //set flags to defaults
     luExtension->ControlStatus = 0; //(can remove, can access)
     luExtension->AudioStatus = 0x15; //no status to return
   #if CACHING
     CDHPCInvalidateCache(luExtension);
   #endif
     luExtension->LuState =LS_INITED;
     HPCDebugPrint(0x20, (0, "\nZeroed modepages for Target=%x Lun=%x, LUext:%lx, modepages:%lx\n",
                              Srb->TargetId, Srb->Lun, luExtension, &(luExtension->ModePages) ));
   }                             
 
                                     
   Srb->SrbStatus = SRB_STATUS_PENDING;

   switch (Srb->Function) {

       case SRB_FUNCTION_ABORT_COMMAND:

           HPCDebugPrint(0x20, (0, "ABORT COMMAND.\n"));
           CDHPCAbort(deviceExtension, luExtension, Srb);
           //
           // Adapter ready for next request.
           //
           ScsiPortNotification(NextRequest,
                                deviceExtension,
                                NULL);
           break;

       case SRB_FUNCTION_RESET_BUS:

           HPCDebugPrint(0x20, (0, "RESET BUS.\n"));
           //
           // Reset CDHPC and SCSI bus.
           //
           CDHPCResetBus(deviceExtension, Srb->PathId);
           Srb->SrbStatus = SRB_STATUS_SUCCESS;
           //
           // "next request" notification is handled in CDHPCResetBus
	   //

           break;

       case SRB_FUNCTION_EXECUTE_SCSI:

           HPCDebugPrint(0x20, (0, "EXECUTE SCSI.\n"));

           //
           // Setup the context for this target/lun.
           //
           luExtension->ActiveLuRequest = Srb;
           /*luExtension->LuState = LS_SELECT;
           luExtension->SavedDataPointer = (ULONG) Srb->DataBuffer;
           luExtension->SavedDataLength = Srb->DataTransferLength;*/

           //
           // Initiate a SCSI request.
           //
           CDHPCStartExecution(deviceExtension, luExtension, Srb);
           break;

       default:

           Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
           ScsiPortNotification(RequestComplete,
                                (PVOID) deviceExtension,
                                Srb);
           //
           // Adapter ready for next request.
           //
           ScsiPortNotification(NextRequest,
                                deviceExtension,
                                NULL);
           break;

   }

   return TRUE;

} // end CDHPCStartIo()


/*************************************************************************
* Routine Name:
*
*    CDHPCInitialize
*
* Routine Description:
* 
*    Inititialize CDHPC adapter. Unfortunately LUExtension data isn't yet 
     set up so we can't initialise the Mode Sense tables here. It gets done in StartIO
     (the first time an LU is accessed) instead.
     
     Note that CD's are allocated to target SCSI IDs starting with target 0 Lun 0-7 for 
     first 8 drives, then target2 etc. There is currently no restriction. This may be 
     altered later to 16 or 24 CD ROMs
* 
* Arguments:
* 
*    Context - Adapter object device extension.
* 
* Return Value:
* 
*    Status.
* 
*/
BOOLEAN
CDHPCInitialize(
   IN PVOID Context
   )
{
   PSPECIFIC_DEVICE_EXTENSION deviceExtension = Context;
   PSPECIFIC_LU_EXTENSION      luExtension;
   struct CD_INIT_PARAMS   HPCCmdBuf;      /*data to send*/
   struct CD_INIT_RESULT   HPCCDInfo;      /*data to get back*/
   HPC_CALL_PARAMS  HPCCommandBlk;  /*command block for HPC transfer*/
   USHORT status;  

   //
   // Check HPC status. Not sure what we do if it isn't ready!
   // should also send 0300:0000 to CD to init it...
   //

   HPCDebugPrint(0x20, (0, "\nCDHPCInit Entered...\n"));
   
   status = HPC_CheckStatus(deviceExtension);
   if (status && HPCST_READY) 
   {
     HPCDebugPrint(0x20, (0, "CDHPCInit found HPC ready...\n"));
     // check for CDROMs
     //set up command block - command in HPCCmdBuf, result returned to HPCSYSInfo
     //set CDinit command: 0300:0000
     HPCCmdBuf.hpc_id = HPC_CD_ID; HPCCmdBuf.reason = HPC_CD_INIT;

     HPCCommandBlk.Interrupt_flag = INTERRUPTFLG;
     HPCCommandBlk.Buff_flag = HPC_BUFFER_B;
     HPCCommandBlk.Send_Len_1 = sizeof(HPCCmdBuf); //4bytes
     HPCCommandBlk.Send_Ptr_1 = &HPCCmdBuf;
     HPCCommandBlk.Send_Len_2 = 0; 
     HPCCommandBlk.Send_Ptr_2 = NULL;
     HPCCommandBlk.Ret_Len_1 = sizeof(HPCCDInfo); //6bytes
     HPCCommandBlk.Ret_Ptr_1 = &HPCCDInfo;
     HPCCommandBlk.Ret_Len_2 = 0;
     HPCCommandBlk.Ret_Ptr_2 = NULL;
     
     HPCExecCall (deviceExtension, HPCCommandBlk);
     
     CDHPCShowData (&HPCCDInfo, sizeof(HPCCDInfo));
     
     if (HPCCDInfo.status == CDERR_OK)
     {
       deviceExtension->NumCDDrives=HPCCDInfo.ndrives;
       // temporarily noble driver to only do one CD
       // if (HPCCDInfo.ndrives >= 1) deviceExtension->NumCDDrives = 1;
       if ( HPCCDInfo.ndrives == 0) /* no drives */
       {
         HPCDebugPrint(0x20, (0, "CDHPCInit: No CDROM found...\n" ));
         return FALSE;
       }
       else  /* init OK */
       {
         HPCDebugPrint(0x20, (0, "CDHPCInit found %d CD-ROMs...\n",HPCCDInfo.ndrives ));
         return TRUE;
       }
     }
     else /* HPC not ready - does this matter? */
     {
       HPCDebugPrint(0x20, (0, "CDHPCInit error from CD: %x ...\n",HPCCDInfo.status ));
       return FALSE;
     }
         
   }
   else 
   {
     HPCDebugPrint(0x20, (0, "CDHPCInit found HPC _not_ ready...\n"));
     return FALSE;
   }

} // end CDHPCInitialize()

/*****************************************************************
* HPCCmdSetup - not yet used
*
* Fills in a standard HPC command block with 
* interrupts on/off flag, buffer /b flag 
* then pointer and size for send1, send2, return1, return2
*
* This is basically just a macro.
*/

VOID 
HPCCmdSetup(
   IN OUT HPC_CALL_PARAMS *HPCCommandBlk,
   IN     UCHAR            interruptflag,   IN     UCHAR            bufferflag,
   IN     WORD             Send1Length,     IN     PVOID            Send1,
   IN     WORD             Send2Length,     IN     PVOID            Send2,
   IN     WORD             Return1Length,   IN     PVOID            Return1,
   IN     WORD             Return2Length,   IN     PVOID            Return2
   )
{   
     HPCCommandBlk->Interrupt_flag = interruptflag;
     HPCCommandBlk->Buff_flag = bufferflag;
     HPCCommandBlk->Send_Len_1 = Send1Length;
     HPCCommandBlk->Send_Ptr_1 = Send1;
     HPCCommandBlk->Send_Len_2 = Send2Length; 
     HPCCommandBlk->Send_Ptr_2 = Send2;
     HPCCommandBlk->Ret_Len_1 = Return1Length;
     HPCCommandBlk->Ret_Ptr_1 = Return1;
     HPCCommandBlk->Ret_Len_2 = Return2Length;
     HPCCommandBlk->Ret_Ptr_2 = Return2;
}


/*****************************************************************
* HPCExecCall
*
* Does a complete HPC transaction given some data in a Call_parms structure
*
*/
ULONG
HPCExecCall(
   IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
   IN HPC_CALL_PARAMS            HPCCmdBlk
   )	
{
	USHORT i,status;
	PUCHAR SharedmemBuf= deviceExtension->MemAddress[HPCCmdBlk.Buff_flag];
	PUCHAR p;

  // check sent data blocks are not too long >16K if so return with 'bad length'
  /* missing*/

  //read status port, update RAM copy, wait until 'ready'
  do
  {
     status = HPC_CheckStatus(deviceExtension);
     HPCDebugPrint(0x002, (0, "HPCExec_call:status=%x\n",status ));
    
  }
  while (!(status & HPCST_READY));
  
  
  //force buf flag to 1 if greater than 1 - ie buff B
  /* missing*/
  
  /* copymem takes <to> <from> <len> - should probably convert addresses to scsi-speak*/
  //if 'sharedmem' available 
  {
    char * p;
    p= (char *)HPCCmdBlk.Send_Ptr_1;
  HPCDebugPrint(0x008, (0, "HPCCDExec: entered, command %x %x\n", p[0], p[2]));
  }
  HPCDebugPrint(0x004,  (0, "HPCCDExec: Command block:\nBuffer:%x\nSend_ptr_1:%lx\nSend_len_1:%x\nRet_ptr_1:%lx\nRet_Len_1:%x\n",
           HPCCmdBlk.Buff_flag,HPCCmdBlk.Send_Ptr_1,HPCCmdBlk.Send_Len_1,HPCCmdBlk.Ret_Ptr_1,HPCCmdBlk.Ret_Len_1));
  HPCDebugPrint(0x004,  (0, "Send_ptr_2:%lx\nSend_len_2:%x\nRet_ptr_2:%lx\nRet_Len_2:%x\n",
           HPCCmdBlk.Send_Ptr_2,HPCCmdBlk.Send_Len_2,HPCCmdBlk.Ret_Ptr_2,HPCCmdBlk.Ret_Len_2));

  if (deviceExtension->SharedmemAvailable)
  {
    HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_WRITE_MEM + HPCCmdBlk.Buff_flag);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent writeMem command:%x\n",HPC_WRITE_MEM + HPCCmdBlk.Buff_flag));
    HPCDebugPrint(0x002, (0, "HPCCDExec: SharedMEM buffer at:%lx\n",SharedmemBuf));
    //copy memory from send_ptr_1 for send_len_1 to sharedmem
    memcpy (SharedmemBuf,                      HPCCmdBlk.Send_Ptr_1, HPCCmdBlk.Send_Len_1);
    //copy memory from send_ptr_2 for send_len_2 to sharedmem+send_len_1
    memcpy (SharedmemBuf+HPCCmdBlk.Send_Len_1, HPCCmdBlk.Send_Ptr_2, HPCCmdBlk.Send_Len_2);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent data\n"));
   
    HPC_do_n_wait(deviceExtension, HPCCmdBlk.Buff_flag, HPCCmdBlk.Interrupt_flag);
    
    HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_READ_MEM + HPCCmdBlk.Buff_flag);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent readMem command:%x\n",HPC_READ_MEM + HPCCmdBlk.Buff_flag) );
    //copy memory from Ret_ptr_1 for Ret_len_1 from sharedmem
    memcpy (HPCCmdBlk.Ret_Ptr_1, SharedmemBuf,                     HPCCmdBlk.Ret_Len_1);
    //copy memory from Ret_ptr_2  for Ret_len_2 from sharedmem+ret_len_1
    memcpy (HPCCmdBlk.Ret_Ptr_2, SharedmemBuf+HPCCmdBlk.Ret_Len_1, HPCCmdBlk.Ret_Len_2);
  }
  
  else /* no sharedmem  - use ports*/
  /* this would be faster if we used ScsiPortreadPortBufferUshort to read buffer */
  {
    HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_WRITE_IO + HPCCmdBlk.Buff_flag);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent writeIO command:%x\n",HPC_WRITE_IO + HPCCmdBlk.Buff_flag));
    //copy memory from send_ptr_1 for send_len_1 to portA or B
    for (i=0, p=HPCCmdBlk.Send_Ptr_1; i < HPCCmdBlk.Send_Len_1; i++)
          HPC_WRITE_DATA (deviceExtension->BaseAddress, p[i], HPCCmdBlk.Buff_flag);
    //copy memory from send_ptr_2 for send_len_2 to HPC data port
    for (i=0, p=HPCCmdBlk.Send_Ptr_2; i < HPCCmdBlk.Send_Len_2; i++)
          HPC_WRITE_DATA (deviceExtension->BaseAddress, p[i],HPCCmdBlk.Buff_flag);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent data\n"));
   
    HPC_do_n_wait(deviceExtension, HPCCmdBlk.Buff_flag, HPCCmdBlk.Interrupt_flag);
    
    HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_READ_IO + HPCCmdBlk.Buff_flag);
    HPCDebugPrint(0x002, (0, "HPCCDExec: sent readIO command:%x\n",HPC_READ_IO + HPCCmdBlk.Buff_flag) );
    //copy memory from Ret_ptr_1 for Ret_len_1 from HPC data port
    for (i=0, p=HPCCmdBlk.Ret_Ptr_1; i < HPCCmdBlk.Ret_Len_1; i++)
          p[i] = HPC_READ_DATA (deviceExtension->BaseAddress, HPCCmdBlk.Buff_flag);
    //copy memory from Ret_ptr_2 for Ret_len_2 from HPC data port
    for (i=0, p=HPCCmdBlk.Ret_Ptr_2; i < HPCCmdBlk.Ret_Len_2; i++)
          p[i] = HPC_READ_DATA (deviceExtension->BaseAddress, HPCCmdBlk.Buff_flag);
  }
  p=HPCCmdBlk.Ret_Ptr_1;
  HPCDebugPrint(0x004, (0, "HPCCDExec: Returned data:\nRet_ptr_1:%x %x %x %x %x %x %x %x\n",
           p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]));

  
  // tell it we've finished
  HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_RELEASE_BUF + HPCCmdBlk.Buff_flag);
  HPCDebugPrint(0x002, (0, "HPCCDExec: sent Release buffer command\n", HPC_RELEASE_BUF + HPCCmdBlk.Buff_flag ));
  
}   

/*****************************************************************
* HPC_do_n_wait
*
* Starts an HPC transaction and waits for it to be done
*  currently polls status port, but should prob be changed to interrupt
*
* on entry:
*         buffer is set to 0 for buffer A, 1 for buffer B
* on exit: 
*         HPC mechanism is ready to continue - ie job done.
*/
void HPC_do_n_wait(IN PSPECIFIC_DEVICE_EXTENSION deviceExtension,
                                            BYTE buffer,
                                            BYTE useinterrupts )
{
  BYTE status; 
  
  HPCDebugPrint(0x002, (0, "HPCdo_n_wait entered\n"));
  //set off the command
  HPC_WRITE_COMMAND (deviceExtension->BaseAddress, HPC_DO_CALL + buffer + useinterrupts);
  
  //then wait till it's done
  do
  {
    status = HPC_CheckStatus(deviceExtension);
    HPCDebugPrint(0x002, (0, "HPCdo_n_wait:status=%x\n",status ));
    
  }
  while (!(status & HPCST_READY));
  
}

/************************************************
* HPC_CheckStatus
*
* reads the status from the HPC port, and updates the software copy
*/

BYTE HPC_CheckStatus (  PSPECIFIC_DEVICE_EXTENSION  deviceExtension ) 
{
    deviceExtension->HPCstatus  = HPC_READ_STATUS (deviceExtension->BaseAddress);
    return deviceExtension->HPCstatus;
}


/*************************************************************************
* Routine Name:
*
*    CDHPCDetermineInstalled
*
* Routine Description:
* 
*    This routine is called by CDHPCFindAdapter and attempts to find
*    the adapter boards in the system by checking if HPC is working.
*    If an adapter is found, the BaseAddress is initialized.
* 
* Arguments:
* 
*    DeviceExtension -   The device specific context for the call.
*    ConfigInfo      -   Pointer to the configuration information structure to
*                        be filled in.
*    AdapterCount    -   Supplies the count of adapter slots which have been tested.
*    Again           -   Returns back a request to call this function again.
* 
* Return Value:
* 
*    SP_RETURN_FOUND     - if an adapter is found.
*    SP_RETURN_NOT_FOUND - if no adapter is found.
* 
*/
ULONG
CDHPCDetermineInstalled(
   PSPECIFIC_DEVICE_EXTENSION              DeviceExtension,
   IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
   IN OUT PULONG                           AdapterCount,
   OUT PBOOLEAN                            Again
   )
{
   PUCHAR         baseAddress;
   PUCHAR	      memAddress; //should this be a PUCHAR? or something longer?
   PUCHAR         ioSpace;
   ULONG          i;
   ULONG          j;
   ULONG          basePort;
   ULONG          numPorts;
   HPC_SYSINFO_PARAMS   HPCCmdBuf;      /*data to send*/
   HPC_SYSINFO_RESULT   HPCSysInfo;     /*data to get back*/
   HPC_CALL_PARAMS      HPCCommandBlk;  /*command block for HPC transfer*/

	// Honor passed-in config info for Chicago, and only check
	// that port.
    
     basePort = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
    
     if (basePort != 0) numPorts = 1;
     // If nothing passed-in, set defaults...
     else 
     {
       HPCDebugPrint(0x40, (0, "CDHPCDetInst:Using default addresses 300-307\n"));
       basePort = HPC_PORT_FIRST;
       numPorts = HPC_PORTS_WIDTH;
     }
    
     baseAddress = ScsiPortGetDeviceBase(
              DeviceExtension,
              ConfigInfo->AdapterInterfaceType,
              ConfigInfo->SystemIoBusNumber,
              ScsiPortConvertUlongToPhysicalAddress(basePort),
              HPC_PORTS_WIDTH,       // NumberOfBytes
              (BOOLEAN) TRUE);  // InIoSpace

     DeviceExtension->BaseAddress = baseAddress;

     // Do sysinfo command to see if HPC & Sharedmemory are available 
	
     // Use ports until we know otherwise
     DeviceExtension->SharedmemAvailable=FALSE;
 
     //set up command block - command in HPCCmdBuf, result returned to HPCSYSInfo
     //set identify command: 0000:0000
     HPCCmdBuf.hpc_id = 0; HPCCmdBuf.reason = 0;
     
     HPCCommandBlk.Buff_flag = HPC_BUFFER_A;
     HPCCommandBlk.Interrupt_flag = 0;     /* cannot use interrupts at this stage */
     HPCCommandBlk.Send_Len_1 = sizeof(HPCCmdBuf); //4bytes
     HPCCommandBlk.Send_Ptr_1 = &HPCCmdBuf;
     HPCCommandBlk.Send_Len_2 = 0;
     HPCCommandBlk.Send_Ptr_2 = NULL;
     HPCCommandBlk.Ret_Len_1 = sizeof(HPCSysInfo); //16bytes
     HPCCommandBlk.Ret_Ptr_1 = &HPCSysInfo;
     HPCCommandBlk.Ret_Len_2 = 0;
     HPCCommandBlk.Ret_Ptr_2 = NULL;
     
     HPCExecCall (DeviceExtension, HPCCommandBlk);
     
     CDHPCShowData(&HPCSysInfo, sizeof(HPCSysInfo));
     
     HPCDebugPrint(0x40, (0, "CDHPCDetInst:HPCIdent=%lx\n",HPCSysInfo.identifier));
     //return buffer should contain identify info,
     if (HPCSysInfo.identifier==0xEE435048) /*"HPC\0xEE"*/
     { /* HPC mechanism identified */
       
       //we should check version here and fail if its incompatible - just report it for now
       if (HPCSysInfo.ver_major != 1)
       {
         HPCDebugPrint(0x40, (0, "CDHPCDetInst:Incorrect HPC version(%d.%d)\n",
                                                  HPCSysInfo.ver_major,HPCSysInfo.ver_minor));
       }
       
       // We found an adapter at this baseAddress.
       //
       *Again = FALSE; // Get it working with one adapter

       // Fill in the access array information.
       // IO first
       (*ConfigInfo->AccessRanges)[0].RangeStart =
           ScsiPortConvertUlongToPhysicalAddress((ULONG)baseAddress);
       (*ConfigInfo->AccessRanges)[0].RangeLength = HPC_PORTS_WIDTH;
       (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //set number of address ranges to 1 - IO only update if sharedmem found
        ConfigInfo->NumberOfAccessRanges   = 1;
       
       HPCDebugPrint(0x40, (0, "CDHPCFindAdp(IO):RangeStrt=%x\n",(*ConfigInfo->AccessRanges)[0].RangeStart));
       HPCDebugPrint(0x40, (0, "CDHPCFindAdp(IO):RangeLen=%x\n",(*ConfigInfo->AccessRanges)[0].RangeLength));
       
       // If Sharememory present then register it
       HPCDebugPrint(0x40, (0, "CDHPCFindAdp(MEM):flag from buffer A =%lx\n", HPCSysInfo.buf_A_physaddr));
       //this seems to be being truncated from EC0000 to C000 - check out why.
       /*// nobble sharedmem for first release
       HPCSysInfo.buf_A_physaddr = 0xFFFFFFFF;*/
       if (HPCSysInfo.buf_A_physaddr != 0xFFFFFFFF)
       {
         //set sharedmeminuse for this device
         DeviceExtension->SharedmemAvailable=TRUE;
         //set number of address ranges to 3 - one IO, 2 memory buffers
         ConfigInfo->NumberOfAccessRanges   = 3;
         
         //buffer A
         memAddress = ScsiPortGetDeviceBase(
                    DeviceExtension,
                    ConfigInfo->AdapterInterfaceType,
                    ConfigInfo->SystemIoBusNumber,
                    ScsiPortConvertUlongToPhysicalAddress(HPCSysInfo.buf_A_physaddr),
                    HPC_BUF_LEN,       // NumberOfBytes
                    (BOOLEAN) FALSE);  // NotInIoSpace

         DeviceExtension->MemAddress[0] = memAddress;
         
         (*ConfigInfo->AccessRanges)[1].RangeStart =
             ScsiPortConvertUlongToPhysicalAddress(HPCSysInfo.buf_A_physaddr);
         (*ConfigInfo->AccessRanges)[1].RangeLength = HPC_BUF_LEN;
         (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;
         
         // normally EC000 on gemini
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp(MEMBUFA):Phyaddr=%lx\n",HPCSysInfo.buf_A_physaddr));
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp(MEMBUFA):RangeStrt=%lx\n",(*ConfigInfo->AccessRanges)[1].RangeStart));
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp:(MEMBUFA)RangeLen=%lx\n",(*ConfigInfo->AccessRanges)[1].RangeLength));

         //buffer B
         memAddress = ScsiPortGetDeviceBase(
                    DeviceExtension,
                    ConfigInfo->AdapterInterfaceType,
                    ConfigInfo->SystemIoBusNumber,
                    ScsiPortConvertUlongToPhysicalAddress(HPCSysInfo.buf_B_physaddr),
                    HPC_BUF_LEN,       // NumberOfBytes
                    (BOOLEAN) FALSE);  // NotInIoSpace

         DeviceExtension->MemAddress[1] = memAddress;
         (*ConfigInfo->AccessRanges)[2].RangeStart =
             ScsiPortConvertUlongToPhysicalAddress(HPCSysInfo.buf_B_physaddr);
         (*ConfigInfo->AccessRanges)[2].RangeLength = HPC_BUF_LEN;
         (*ConfigInfo->AccessRanges)[2].RangeInMemory = TRUE;
         
         //normally E8000 on gemini
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp(MEMBUFB):Phyaddr=%lx\n",HPCSysInfo.buf_B_physaddr));
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp(MEMBUFB):RangeStrt=%lx\n",(*ConfigInfo->AccessRanges)[2].RangeStart));
         HPCDebugPrint(0x40, (0, "CDHPCFindAdp:(MEMBUFB)RangeLen=%lx\n",(*ConfigInfo->AccessRanges)[2].RangeLength));
       }
         //set number of address ranges to 1 - IO only
         ConfigInfo->NumberOfAccessRanges   = 1;


       *(AdapterCount)++;

       return(SP_RETURN_FOUND);
     
     }
     else
     { /* No HPC - probably not a PC card */
       HPCDebugPrint(0x40, (0, "\nCDHPCDermineInstalled:HPCIdent=%lx NOT OK\n",HPCSysInfo.identifier));
       // No adapters found. 
       // No need to call again.
       // Clear the adapter count for the next bus.
       ScsiPortFreeDeviceBase( DeviceExtension, ioSpace );
       *Again = FALSE;
       *(AdapterCount) = 0;

       return(SP_RETURN_NOT_FOUND);
    }
       

} // end CDHPCDetermineInstalled()


/*************************************************************************
* Routine Name:
*
*    CDHPCFindAdapter
*
* Routine Description:
*    Called by the OS-specific port driver after the necessary storage has
*    been allocated, to gather information about the adapter's configuration.
*       
* Arguments:
* 
*    DeviceExtension -   The device specific context for the call.
*    Context   -         Passed through from the driver entry as additional
*                        context for the call.
*    BusInformation  -   Unused.
*    ArgumentString  -   Points to the potential IRQ for this adapter.
*    ConfigInfo      -   Pointer to the configuration information structure to
*                        be filled in.
*    Again           -   Returns back a request to call this function again.
* 
* Return Value:
* 
*    SP_RETURN_FOUND     - if an adapter is found.
*    SP_RETURN_NOT_FOUND - if no adapter is found.
* 
*/
ULONG
CDHPCFindAdapter(
   PSPECIFIC_DEVICE_EXTENSION              DeviceExtension,
   IN PVOID                                Context,
   IN PVOID                                BusInformation,
   IN PCHAR                                ArgumentString,
   IN OUT PPORT_CONFIGURATION_INFORMATION  ConfigInfo,
   OUT PBOOLEAN                            Again
   )
{
   PSPECIFIC_DEVICE_EXTENSION  deviceExtension = DeviceExtension;
   ULONG status;

   HPCDebugPrint(0x20, (0, "\nFindAdapter Entered...\n"));

   /* this sets up the accessranges needed*/
   status = CDHPCDetermineInstalled(deviceExtension,
               ConfigInfo,
               Context,
               Again);

   if (status != SP_RETURN_FOUND) {
       return(status);
   }

   // Determine IRQ
   // Note: In future release of NT (after .404 March Beta), this value
   //       can be returned as 0 and NT will assume we are polling and won't
   //       reserve an IRQ for us. Until then, we need to give him something.
  // ConfigInfo->BusInterruptLevel = 0;
   
   // we know what the IRQ is, so set it to that
#if USEINTERRUPTS
   ConfigInfo->BusInterruptLevel = HPC_IRQ_LINE;
#else
   ConfigInfo->BusInterruptLevel = 0;
#endif   
 
   HPCDebugPrint(0x40, (0, "CDHPC:IRQ=%x\n", ConfigInfo->BusInterruptLevel));

   ConfigInfo->Dma32BitAddresses = FALSE;
   ConfigInfo->NumberOfBuses = 1;
   ConfigInfo->InitiatorBusId[0] = SCSI_INITIATOR_ID; /* SCSI ID of this 'interface' =7 */
   ConfigInfo->MaximumTransferLength = HPC_BUF_LEN;
   ConfigInfo->NumberOfPhysicalBreaks = 1;
   ConfigInfo->AlignmentMask = 0x03; /* align to 4 bytes  (I hope that what it means!) */

   HPCDebugPrint(0x20, (0, "Exit FindAdapter.\n"));

   return(SP_RETURN_FOUND);

} // end CDHPCFindAdapter()



/*************************************************************************
* Routine Name:
*
*    DriverEntry
*
* Routine Description:
* 
*    Driver initialization entry point for system.
* 
* Arguments:
* 
*    DriverObject - The driver specific object pointer
*    Argument2    - not used.
* 
* Return Value:
* 
*    Status from ScsiPortInitialize()
* 
*/
ULONG
DriverEntry(
   IN PVOID DriverObject,
   IN PVOID Argument2
   )
{
   HW_INITIALIZATION_DATA  hwInitializationData;
   ULONG                   i;
   ULONG                   rc;

   HPCDebugPrint(0x20, (0, "\nAleph One CDROM Miniport Driver\n"));

   // Zero out the hwInitializationData structure.
   for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++) {

       *(((PUCHAR)&hwInitializationData + i)) = 0;
   }

   hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

   // Set entry points.
   hwInitializationData.HwInitialize   = CDHPCInitialize;
   hwInitializationData.HwStartIo      = CDHPCStartIo;
#if USEINTERRUPTS
   hwInitializationData.HwInterrupt    = CDHPCInterrupt;
#else
   hwInitializationData.HwInterrupt    = NULL;  /*CDHPCInterrupt when used...*/
#endif
   
   hwInitializationData.HwFindAdapter  = CDHPCFindAdapter;
   hwInitializationData.HwResetBus     = CDHPCResetBus;

   // Specify size of device extension.
   hwInitializationData.DeviceExtensionSize = sizeof(SPECIFIC_DEVICE_EXTENSION);

   // Specify size of logical unit extension.
   hwInitializationData.SpecificLuExtensionSize = sizeof(SPECIFIC_LU_EXTENSION);

   hwInitializationData.NumberOfAccessRanges   = 3; /* one IO, two memory */

   hwInitializationData.MapBuffers             = TRUE; /* do we need both of these? */
   hwInitializationData.NeedPhysicalAddresses  = TRUE;

   hwInitializationData.TaggedQueuing          = FALSE;
   hwInitializationData.AutoRequestSense       = FALSE; //this one could be useful..
   hwInitializationData.MultipleRequestPerLu   = FALSE;
   hwInitializationData.ReceiveEvent           = FALSE;

   // The fourth parameter below (i.e., "i") will show up as the
   // "Context" parameter when FindAdapter() is called.

   HPCDebugPrint(0x20, (0, "Trying ISA...\n"));
   hwInitializationData.AdapterInterfaceType = Isa;

   i = 0;
   rc = (ScsiPortInitialize(DriverObject,
                          Argument2,
                          &hwInitializationData,
                          &(i) ) );
                          

   HPCDebugPrint(0x20, (0, "Exit DriverEntry. rc=%x\n", rc));

   return(rc);

} // end DriverEntry()
