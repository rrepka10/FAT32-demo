/*---------------------------------------------------------------------------
  cmpe240.h
  Common include file for ECE340 labs
  07/07/2013 - Initial version
  07/08/2013 - added RLE bmp content
  07/10/2013 - added timer functionality & math
  07/28/2013 - added floating point
  09/18/2013 - change video pointer variable
  10/13/2013 - added UART speeds
  11/09/2013 - Added MFC support
  11/21/2013 - Integrated FAT32.h, pass bytes generated, 
               MFC uint64_t type and int64_t type
  11/25/2013 - Added decStr()
  12/14/2014 - Added booth
  03/16/2014 - Updated comments
  04/17/2014 - Fix for BMP RLE VS 11, made VS uint_8t = unsigned char
  05/14/2014 - Added dummy() function, removed mmio macros
  07/04/2014 - Added DrawString() function, updated delay
  08/30/2014 - Added B+ defines
  10/05/2014 - Switched uart_putc() to const char type, drawPixel() to uint16_t
  12/15/2014 - Added command line data structures
  03/23/2015 - Unified cylinder/head structure
  04/05/2015 - Added enable_irq()
---------------------------------------------------------------------------*/
 
#ifndef CMPE240_H
#define CMPE240_H


#define _MFC_VER 1   // disable for RPI

#define uint8_t     unsigned char
#define uint16_t    unsigned short
#define uint32_t    unsigned long
#define int32_t     long
#define int64_t     __int64
#define uint64_t    unsigned __int64

#define WORD     uint16_t
#define DWORD    uint32_t
#define LONG     int32_t
#define BYTE     uint8_t
#define IEEE_FLT uint32_t




#pragma pack(push, 1)
typedef struct {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  DWORD biSize;     
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)



/*---------------------------------------------------------------------------
  Helper Function prototypes
---------------------------------------------------------------------------*/
int strCopy(char *dest, const char *src);
void fatMemmove(uint8_t*dest, uint8_t *src, uint32_t bytes);
int fatStrcmp(const char *str1, const char *str2);


/*-------------------------------------------------------------------------
                                Defines
-------------------------------------------------------------------------*/
#define NUL                     0
#define LF                      0x0A
#define CR                      0x0D
#define MAXFLEN                 512
#define WRITESIZE               1024*1024

#define MBR_BOOTFLAG_ACTIVE     0x80
#define MBR_BOOTFLAG_INACTIVE   0x00


/*-------------------------------------------------------------------------
  Popular Partition types
-------------------------------------------------------------------------*/
#define PART_TYPE_UNKNOWN       0x00
#define PART_TYPE_12bit_FAT     0x01
#define PART_TYPE_16_bit_SFAT   0x04    // Partition Smaller than 32MB
#define PART_TYPE_Ext_MSDOS_FAT 0x05    // Extended MSDOS partition
#define PART_TYPE_16_bit_LFAT   0x06    // Partition Larger than 32MB
#define PART_TYPE_32_bit_FAT    0x0b    // Partition Up to 2048GB
#define PART_TYPE_32_LBA_FAT    0x0c    // Same as 0BH, uses LBA1 13h Extensions
#define PART_TYPE_16_LBA_FAT    0x0e    // Same as 06H, uses LBA1 13h Extensions
#define PART_TYPE_EXT_LBA_FAT   0x0f    // Same as 05H, uses LBA1 13h Extensions

/*-----------------------------------------------------------------------------
  FAT32 Specific defines
-----------------------------------------------------------------------------*/
#define FILE_HEADER_BLANK               0x00    //• #define’s for first byte(DIR_Name[0]
#define FILE_HEADER_DELETED             0xE5    //• #define’s for first byte(DIR_Name[0]
#define FILE_ATTR_READ_ONLY             0x01
#define FILE_ATTR_HIDDEN                0x02
#define FILE_ATTR_SYSTEM                0x04
#define FILE_ATTR_SYSHID                0x06
#define FILE_ATTR_VOLUME_ID             0x08
#define FILE_ATTR_DIRECTORY             0x10
#define FILE_ATTR_ARCHIVE               0x20
#define FILE_ATTR_LFN_TEXT              0x0F

#define FILE_TYPE_DIR                   0x10
#define FILE_TYPE_FILE                  0x20
#define SIGNATURE_VALUE                 0xAA55
#define FAT32_LAST_CLUSTER              0x0FFFFFF8      // or larger
#define FAT32_INVALID_CLUSTER           0xFFFFFFFF  
#define SECTOR_SIZE                     512
#define SHORT_NAME_LEN                  11


/*---------------------------------------------------------------------------
 Sample partition description:
                                                    00 01   
0001C0:  01 00 0C FE 3F 06 3F 00  00 00 C1 E8 01 00 
                                                    00 00   
   1d0   00 00 00 00 00 00 00 00  00 00 00 00 00 00 
                                                    00 00
   1e0   00 00 00 00 00 00 00 00  00 00 00 00 00 00 
                                                    00 00
0001F0:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 
                                                    55 AA   signature
Partition 1 description from above 
            00     01 01 00       0C     FE 3F 06  3F 00  00 00   C1 E8 01 00
        boot flag  CHS begin  type code  chs end     LBA begin    num sectors
                  should be             0x003f        big endian
                              0b or oc              FAT start        61 MB
                            is 0x7e00

Note: HS is encoded in the 16 bit word 
    Cyl Bits [7:0] | Cyl bits [9:8] | Sector [5:0]
---------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint8_t bootFlag;       // 0x80 - bootable, 0x00 not bootable
    uint8_t  Head_Begin;       
    uint16_t CylSec_Begin; 
    uint8_t typeCode;       // Type code
    uint8_t  Head_End;         
    uint16_t CylSec_End;        
    uint32_t LBA_Begin;     // Starting LBA for this logical drive
    uint32_t numSectors;    // Number of sectors in this drive
} partitionDescription;
#pragma pack(pop)



/*---------------------------------------------------------------------------
  The master book record
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)
typedef struct 
{ 
    uint8_t bootCode[0x1be];        // Boots computer
    partitionDescription par [4];   // Up to 4 partitions
    uint16_t signature;             // 0xAA55
} bootRecord; 
#pragma pack(pop)


/*---------------------------------------------------------------------------
  FAT 32 Volume ID
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)
typedef struct 
{
    uint8_t start [11]; 
    uint16_t bytesPerSector; 
    uint8_t sectorsPerCluster; 
    uint16_t reservedSectors; 
    uint8_t numFats; 
    uint8_t other1 [19]; 
    uint32_t sectorsPerFat; 
    uint8_t other2 [4]; 
    uint32_t rootCluster;  
    uint8_t other3 [512-50]; 
    uint16_t signature;
}  FAT_VolID;
#pragma pack(pop)


/*---------------------------------------------------------------------------
  Individual directory entry 64 bytes
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)   
typedef struct
{
    char Name[11];
    uint8_t Attr;       // File attributes
    uint8_t NTRes;
    uint8_t CrtTimeTenth;
    uint8_t CrtTime[2];
    uint8_t CrtDate[2];
    uint8_t LstAccDate[2];
    uint16_t FstClusHI;     // First cluster high
    uint8_t WrtTime[2];
    uint8_t WrtDate[2];
    uint16_t FstClusLO;     // First cluster low
    uint32_t FileSize;      // File size
//    uint8_t  other [32];
} FAT_DirEntry;
#pragma pack(pop)



/*---------------------------------------------------------------------------
  Root entry
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)   
typedef struct
{
    FAT_DirEntry dirEntry [8*2];
} FAT_DirSector;
#pragma pack(pop)

/*---------------------------------------------------------------------------
  Each FAT 32 entry is 4 bytes, each sector is always 512
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)   
typedef struct
 {  // 128 entries typical
    uint32_t entry [SECTOR_SIZE/sizeof(uint32_t)];
} FAT_Access;
#pragma pack(pop)


    
/*---------------------------------------------------------------------------
  Global data structure for all FAT access, 
---------------------------------------------------------------------------*/           
#pragma pack(push, 1)   
typedef struct {
    uint32_t fatBeginLBA;       // Start of the FAT linked list
    uint32_t dirBeginLBA;       // Start of the directory
    uint32_t sectorsPerCluster;
    uint32_t bytesPerSector;
} IO_Init;
#pragma pack(pop)

/*---------------------------------------------------------------------------
  Used to open a file
---------------------------------------------------------------------------*/
#pragma pack(push, 1)   
typedef struct {
    char fileName [SHORT_NAME_LEN+2];       // 8.3 names + dot + 0x00
    uint32_t startingCluster;
    uint32_t FileSize;
    uint8_t Attr;
    } FileHandle;
#pragma pack(pop)

//-------------------------------------------------------------------------
//  FAT functions
//-------------------------------------------------------------------------
void *readLBA(uint32_t block);
uint32_t nextFatEntry(uint32_t cluster);
uint32_t readCluster(uint32_t cluster, uint8_t *buffer);
uint32_t initHDD(void);
FAT_DirEntry *searchDir(const char *fileName);
uint32_t fatOpen(const char *fileName, FileHandle *handle);
int fatRead(FileHandle *handle, uint8_t *buffer, uint32_t bufferSize);
void readHDDimg(const char* fileName);

#endif // #ifndef CMPE240_H
