/*************************************************************************
*
* Copyright (c) 1997 Aleph One Ltd
* 
* Module Name:  CDHPC.h
* 
* Miniport Driver for Aleph One PC cards HPC mechanism - CDROM variant
* 
* Environment:  kernel mode only
* 
* v0.01 1997.11.18 Started from win95 sample miniport block driver code
*/


#define HPC_CD_ID 0x0003

#define HPC_CD_INIT       0x0000
#define HPC_CD_READ       0x0001
#define HPC_CD_SEEK       0x0002
#define HPC_CD_PLAY       0x0003
#define HPC_CD_PLAYSTATUS 0x0004
#define HPC_CD_DISKINFO   0x0005
#define HPC_CD_TRACKINFO  0x0006
#define HPC_CD_QCHANNEL   0x0007
#define HPC_CD_CONTROL    0x0008

/* -------------------- */

struct CD_COMMON_PARAMS
{
  WORD hpc_id;
  WORD reason;
};

/* ------------ */

struct CD_COMMON_RESULT
{
  ULONG retcode;
};

/* ------------ */

struct CD_INIT_PARAMS
{
  WORD hpc_id;
  WORD reason;
};

/* ------------ */

struct CD_INIT_RESULT
{
  WORD status;
  WORD audiostat;
  WORD ndrives;
};

/* ------------ */

#define CD_MAX_RD_LEN (16384-16)
#define CD_READ_MAXSECTS 7

struct CD_READ_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
  WORD sect_count;
  ULONG sect_start;
  WORD readahead;
};

/* ------------ */

struct CD_READ_RESULT
{
  WORD status;
  WORD audiostat;
  /*the read data comes next*/ 
};

/* ------------ */

struct CD_SEEK_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
  WORD nsects;
  ULONG sect_start;
};

/* ------------ */

struct CD_SEEK_RESULT
{
  WORD status;
  WORD audiostat;
};

/* ------------ */

struct CD_PLAY_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
  WORD pad2;
  ULONG sect_start;
  ULONG play_len;
};

/* ------------ */

struct CD_PLAY_RESULT
{
  WORD status;
  WORD audiostat;
};

/* ------------ */

struct CD_PLAYSTATUS_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
};

/* ------------ */

struct CD_PLAYSTATUS_RESULT
{
  WORD status;
  WORD audiostat;
  ULONG playstart;
  ULONG playpos;
  ULONG playstop;
};

/* ------------ */

struct CD_DISKINFO_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
};

/* ------------ */

struct CD_DISKINFO_RESULT
{
  WORD status;
  WORD audiostat;
  ULONG disksize;
};

/* ------------ */

struct CD_TRACKINFO_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
  WORD trknum;
#define TRKNUM_WHOLE_CD 0xFFFF
};

/* ------------ */

struct CD_TRACKINFO_RESULT
{
  WORD status;
  WORD audiostat;
  ULONG trkstart;
  BYTE firsttrk;
  BYTE lasttrk;
  BYTE trkflags;
#define TRKFLAGS_DATA 0x40
#define TRKFLAGS_4CH  0x80
};

/* ------------ */

struct CD_QCHANNEL_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
};

/* ------------ */

struct CD_QCHANNEL_RESULT
{
  WORD status;
  WORD audiostat;
  BYTE Qdata[10];
};

/* ------------ */

struct CD_CONTROL_PARAMS
{
  WORD hpc_id;
  WORD reason;
  WORD drvnum;
  WORD fncode;
#define CONTROL_RESET   0
#define CONTROL_CHANGED 1
#define CONTROL_PAUSE   2
#define CONTROL_RESUME  3
#define CONTROL_UNLOCK  4
#define CONTROL_LOCK    5
#define CONTROL_OPEN    6
#define CONTROL_CLOSE   7
#define CONTROL_NOACTION 0xFFFF
};

/* ------------ */

struct CD_CONTROL_RESULT
{
  WORD status;
  WORD audiostat;
};


/* ------------ */

union CD_HPC_PARAMS
{
struct CD_COMMON_PARAMS     common;
struct CD_INIT_PARAMS       Init;
struct CD_READ_PARAMS       Read;
struct CD_SEEK_PARAMS       Seek;
struct CD_PLAY_PARAMS       Play;
struct CD_PLAYSTATUS_PARAMS PlayStatus;
struct CD_DISKINFO_PARAMS   DiskInfo;
struct CD_TRACKINFO_PARAMS  TrackInfo;
struct CD_QCHANNEL_PARAMS   QChannel;
struct CD_CONTROL_PARAMS    Control;
};

/* ------------ */

union CD_HPC_RESULT
{
struct CD_COMMON_RESULT     common;
struct CD_INIT_RESULT       Init;
struct CD_READ_RESULT       Read;
struct CD_SEEK_RESULT       Seek;
struct CD_PLAY_RESULT       Play;
struct CD_PLAYSTATUS_RESULT PlayStatus;
struct CD_DISKINFO_RESULT   DiskInfo;
struct CD_TRACKINFO_RESULT  TrackInfo;
struct CD_QCHANNEL_RESULT   QChannel;
struct CD_CONTROL_RESULT    Control;
};

/* ------------ */

typedef union
{
  union CD_HPC_PARAMS in;
  union CD_HPC_RESULT out;
}
  CD_HPC_IN_OUT;


/* Error codes ---------------- */

#define HPCERR_NOTPRESENT 0xFFFFFFFF

#define CDERR_OK	0
#define CDERR_BADFUNC	1
#define CDERR_BADDRIVE  2
#define CDERR_BADPARAMS 3
#define CDERR_CHANGED   4
#define CDERR_BUSY	5
#define CDERR_READERR   6
#define CDERR_DRIVEFAIL 7

#define AUDIO_STOPPED   0
#define AUDIO_PLAYING   1
#define AUDIO_PAUSED    2
#define AUDIO_COMPETED  3
#define AUDIO_ERR_STOP  4
#define AUDIO_NODATA    5

//
// SCSI source identifier for host system.
//
#define SCSI_INITIATOR_ID  7


// stuff about cache and block sizes
#define CD_CACHE_SIZE 7 //do same number of blocks as max HPC read

#define CD_BLOCK_SIZE 2048     //this is assumed in several places


/* Mode parameter page structs 
CDROMs support pages 
00: vendor specific - ignore this one
01: read write errors
02: diconnect/reconnect
07: verification error page
08: caching page
09: periperal device page
0A: control mode page
0b: medium type page
0d: cdrom type page
0e: cdrom audio page
*/



typedef struct _MODE_PARAM_PAGES 
{
  MODE_PARAMETER_HEADER header; //overall header
  MODE_PARAMETER_BLOCK block; //optional blocks
  MODE_ERROR_RECOVERY_PAGE errorpg; //page 01
  MODE_DISCONNECT_PAGE disconnectpg; //page 02
  MODE_CACHING_PAGE cachepg; //page 08
  struct _MODE_PERIPHERAL_PAGE //page 09
  {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
  }  periphpg;
  struct _MODE_CONTROL_MODE_PAGE //page 0A
  {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
  }  controlpg; 
  struct _MODE_MEDIUM_PAGE //page 0B
  {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
  } mediumpg; 
  struct _MODE_CDROM_PAGE  //page 0D
  {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    } cdrompg;
  struct _MODE_CDROM_AUDIO_PAGE  //page 0E
  {
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    } cdromaudpg;
} MODE_PARAM_PAGES ;


#define SCSI_MODE_SENSE_PAGE_MASK 0x3F
