/*---------------------------------------------------------------------------
  Windows console demonstration program to read simple FAT32 files
  Revision History:
  07/07/2013 - Initial version
  11/04/2013 - Minor name changes
  03/31/2014 - Added more pseudo code
  12/15/2014 - Changed uart function names
  03/22/2015 - Added fopen_s()
---------------------------------------------------------------------------*/

//-------------------------------------------------------------------------
//  The include files 
//-------------------------------------------------------------------------
 //#include "stdafx.h"				// Uncomment these 2 lines for MSC
 //#include <afx.h>
#include <stdint.h>				// Uncomment this 1 line for RPI
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cmpe240.h"

/*-------------------------------------------------------------------------
  This stores the memory map of the HDD file
-------------------------------------------------------------------------*/
// uint8_t HDDimage [32*1024*1024];
 uint8_t* HDDimage;



/*-------------------------------------------------------------------------
  This will contain global information about the boot drive
-------------------------------------------------------------------------*/
IO_Init MediaDescriptorGlobal = {0, 0, 0, 0};

/*---------------------------------------------------------------------------
  This only reads from the primary boot record
	uint32_t initHDD(void)
  Where:
	IO_Init *io - Pointer to a media description storage buffer which will be
				  updated upon success
  Returns: 0 for success, non-zero for failure
---------------------------------------------------------------------------*/
uint32_t initHDD(void)
{
	// use to link through the book record
	bootRecord *bootRec_p;
	partitionDescription *parDesc_p;
	FAT_VolID  *fatVol_p;

	//-----------------------------------------------------------------------
	// Check the boot record signature
	//-----------------------------------------------------------------------
	bootRec_p = (bootRecord *)readLBA(0);
	if (bootRec_p->signature != SIGNATURE_VALUE)
	{
		
		printf("Invalid boot record\n");
		
		return(99);
	} // End if

	// Access the first partition record
	parDesc_p = &bootRec_p->par[0];

	// Verify this is a FAT 32 LBA partition
	if (parDesc_p->typeCode != PART_TYPE_32_LBA_FAT)
	{
	
		printf("Invalid boot partition type\n");
		
		return(99);
	} // End if bad type

	printf("num Sectors= %d (%04xX) size= %d (MB)\n", parDesc_p->numSectors, 
				parDesc_p->numSectors, parDesc_p->numSectors*512/1024/1024);
	printf("Partition description type is %d (%04xX)\n", parDesc_p->typeCode, 
				parDesc_p->typeCode);
	
   // Read the LBA of the first valid partition
	fatVol_p = (FAT_VolID *)readLBA(parDesc_p->LBA_Begin);
	printf("LBA starts at %04xX\n", parDesc_p->LBA_Begin);
	printf("bytes per sector= %d, sectors per cluster= %d, reserved sectors %d (%04xX)\n",
		fatVol_p->bytesPerSector, fatVol_p->sectorsPerCluster, 	
					fatVol_p->reservedSectors, fatVol_p->reservedSectors);
	printf("number of FATS %d, sectors per fat %04xX, root cluster %04xX, signature %02xX\n",
		fatVol_p->numFats, fatVol_p->sectorsPerFat, fatVol_p->rootCluster, fatVol_p->signature);


   	// Verify the FAT signature
	if (fatVol_p->signature != SIGNATURE_VALUE)
	{
		printf("Invalid FAT Vol record\n");	
		return(100);
	} // End if

	// Fill in the media descriptor
	MediaDescriptorGlobal.sectorsPerCluster = fatVol_p->sectorsPerCluster;
    MediaDescriptorGlobal.bytesPerSector = fatVol_p->bytesPerSector;
	MediaDescriptorGlobal.fatBeginLBA = parDesc_p->LBA_Begin + fatVol_p->reservedSectors;
	MediaDescriptorGlobal.dirBeginLBA = MediaDescriptorGlobal.fatBeginLBA + (fatVol_p->numFats*fatVol_p->sectorsPerFat);

	printf("FAT Begin LBA= %04xX  Directory begin LBA %04xX\n\n", MediaDescriptorGlobal.fatBeginLBA, MediaDescriptorGlobal.dirBeginLBA); 

	return(0);
} // End initHDD

/*---------------------------------------------------------------------------
  Opens a case sensitive file 8.3 file name.
	int32_t fatOpen(char *fileName, FileHandle *handle)
  Where:
	char *fileName     - Pointer to the case specific name
	FileHandle *handle - Pointer to file handle storage block
  Returns 1 for success, 0 for failure
---------------------------------------------------------------------------*/
uint32_t fatOpen(const char *fileName, FileHandle *handle)
{
	// Pointer to the directory entry
	FAT_DirEntry *dirEntry;

	// Try to find the file
	dirEntry = searchDir(fileName);

	// Return 0 if not found
	if (!dirEntry) {return(0);}

	// Populate the structure
	strCopy(handle->fileName, fileName);
	handle->startingCluster = (dirEntry->FstClusHI << 16) | dirEntry->FstClusLO;
	handle->FileSize = dirEntry->FileSize;
	handle->Attr = dirEntry->Attr;

	return(1);
} // End fatOpen



/*---------------------------------------------------------------------------
  This function searches the directory structure one sector at a time.
  It only requires 1 buffer
	FAT_DirEntry *searchDir(const char *fileName)
  Where:
	char *fileName - Pointer to the case specific file name to fine
 Returns: 0 if no entry is found or a pointer to a FAT_DirEntry for success
---------------------------------------------------------------------------*/
FAT_DirEntry *searchDir(const char *fileName)
{
	// General purpose variables
	uint32_t i,entry, index;

	// The directory starts at cluster 
	uint32_t cluster = 0; 

	// Temporarily holds the fixed up file name
	uint8_t zname [SHORT_NAME_LEN+2];

	// Points to the directory LBA
	uint32_t directoryLBA;

	// Points to a directory
	FAT_DirSector *dir_p;
	
	// Point to the start of the directory chain
	directoryLBA = MediaDescriptorGlobal.dirBeginLBA;

	// No entry was found
   while (cluster < FAT32_LAST_CLUSTER)
   {
		// Get the directory entry
		dir_p = (FAT_DirSector *)readLBA(directoryLBA+cluster);

		// process all the directory entries
		for (entry = 0; entry < (sizeof(FAT_DirSector)/sizeof(FAT_DirEntry)); entry++)
		{	
			// The filename is not nul terminated and may have embedded space, remove them
			index = 0;
			for (i = 0; i < SHORT_NAME_LEN; i++) 
			{
				// Ignore space characters
				if (dir_p->dirEntry[entry].Name [i] != ' ') 
				{
					zname [index] = dir_p->dirEntry[entry].Name [i];
					index++;
				}

				// Insert the 8.3 period
				if (i == 7) 
				{
					zname [index] = '.';
					index++;
				}
			} // End for i
			// Put in the null
			zname [index] = 0x00;

			// Does the name match?
			if (fatStrcmp((const char *)&zname, (const char *)fileName) == 0) {
				// Yes
				return(&dir_p->dirEntry[entry]);
			} // end if match
			else {
				if ((zname[0] == FILE_HEADER_BLANK) || (zname[0] == FILE_HEADER_DELETED)) {
					printf("dir: <unused>\n");
				}
				else {
					printf("dir: %s\n", zname);
					}
			}
			
		} // End for entry
		
		//Get the data
  		cluster = nextFatEntry(cluster);
	} // End wile more directories

	// No entry found
	return(0);
} // End searchDir


/*---------------------------------------------------------------------------
  This simulates reading a LBA address by returning a pointer to the
  file memory image read in during media initialization.
	void *readLBA(uint32_t block)
  Where:
	uint32_t block - HDD block to read
  Returns: A the passed buffer pointer
---------------------------------------------------------------------------*/
void *readLBA(uint32_t block)
{
   return(&HDDimage[block*SECTOR_SIZE]);
} // end readLBA


/*---------------------------------------------------------------------------
  This function reads the entire file pointed to by handle into buffer.
	int fatRead(FileHandle *handle, uint8_t *buffer, uint32_t bufferSize)
  Where: 
	FileHandle *handle  - Pointer to an open file handle
	uint8_t *buffer     - Pointer to a buffer to store the file
	uint32_t bufferSize - The size of the buffer
  Returns: -1 for error or the file size for success
---------------------------------------------------------------------------*/
int fatRead(FileHandle *handle, uint8_t *buffer, uint32_t bufferSize)
{
	// Use to follow the cluster chain and index the read buffer
	uint32_t cluster;
	uint32_t readIndex = 0;

	// Make sure the buffer is big enough
	if (bufferSize < handle->FileSize) {return(-1);}

	// Start the cluster chain
	cluster = handle->startingCluster ;
		
	// Read until we find the end of cluster identifier 
	while (cluster < FAT32_LAST_CLUSTER)
		{
		printf("file name= '%s',  cluster %08xX  (not required)\n", handle->fileName, cluster);

		//Get the data
		readIndex += readCluster(cluster, &buffer [readIndex]);
  		cluster = nextFatEntry(cluster);
		}

	// Success, return the size
	return(handle->FileSize);
} // End fatRead()



/*---------------------------------------------------------------------------
  This reads a cluster of a file given the file cluster id and the cluster
  begin area.  Buffer must be at least 512 bytes * number of sectors per cluster
	uint32_t readCluster(uint32_t cluster, uint8_t *buffer)
  Where:
	uint32_t cluster - cluster to read
	uint8_t *buffer  - pointer to the buffer
  Returns: bytes read
---------------------------------------------------------------------------*/
uint32_t readCluster(uint32_t cluster, uint8_t *buffer)
{
		//
		uint8_t *buff_p;
		uint32_t bytesToCopy;
		bytesToCopy = MediaDescriptorGlobal.bytesPerSector*MediaDescriptorGlobal.sectorsPerCluster;


		// Read the logical block in
		buff_p = (uint8_t *)readLBA(MediaDescriptorGlobal.dirBeginLBA + cluster-2);

		// Copy the data into the users buffer
		fatMemmove(buffer, buff_p, bytesToCopy); 
		return(bytesToCopy);
}// End readCluster




/*---------------------------------------------------------------------------
  This follows a 32 FAT chain, no checking is done.  The caller must
  check for less than FAT32_LAST_CLUSTER or FAT32_INVALID_CLUSTER
	uint32_t nextFatEntry(uint32_t cluster)
  Where: 
	uint32_t cluster - Starting FAT cluster
  Returns: the next FAT cluster OR some value between 
			FAT32_LAST_CLUSTER and FAT32_INVALID_CLUSTER for end
			Success is <  FAT32_LAST_CLUSTER

---------------------------------------------------------------------------*/
uint32_t nextFatEntry(uint32_t cluster)
	{
		FAT_Access *FAT_p;

		FAT_p = (FAT_Access *)readLBA(MediaDescriptorGlobal.fatBeginLBA);
		// FAT_Access points to the start of the FAT LBA, cluster is the index in
		return(FAT_p->entry [cluster]);
	} // nextFatEntry


/*---------------------------------------------------------------------------
  This copies a nul terminated string.
	int strCopy(char *dest, char *src)
  Where:
	char *dest - pointer to the destination string
	char *src  - pointer to the nul terminated source string
  Returns: bytes copied.
---------------------------------------------------------------------------*/
int strCopy(char *dest, const char *src)
{
	uint32_t count = 0;
	while (*src != 0x00)
	{
		*dest = *src;
		dest++;
		src++;
		count++;
	} // End while
        
        // Put the nul in
        *dest = 0x00;
	return(count);
} // End strCopy



/*---------------------------------------------------------------------------
  This compares two nul terminated string.  Returns 0 for match.
	int fatStrcmp(char *str1, char *str2)
  Where:
	char *str1 - pointer to the first source string
	char *str2 - pointer to the second source string
  Returns: 0 for match, 1 otherwise
---------------------------------------------------------------------------*/
int fatStrcmp(const char *str1, const char *str2)
{
	
	while ((*str1 != 0x00) && (*str2 != 0x00))
	{
		if (*str1 != *str2) {return(1);}
		str1++;
		str2++;
	} // End while
	if (*str1 != *str2) {return(1);}
	return(0);
} // End fatStrcmp
 


/*---------------------------------------------------------------------------
  This copies bytes from source to destination
	void fatMemmove(char *dest, char *src, uint32_t bytes)
  Where:
	char *dest      - pointer to the destination buffer
	char *src      - pointer to the source buffer
	uint32_t bytes - number of bytes to copy
  Returns: bytes copied.
---------------------------------------------------------------------------*/
void fatMemmove(uint8_t *dest, uint8_t*src, uint32_t bytes)
{
	while (bytes)
	{
		*dest = *src;
		dest++;
		src++;
		bytes--;
	} // End while
	return;
} // End fatMemmove




void readHDDimg(const char *fileName) {
		
	FILE* myFile;
	int rc;
	size_t size;
	struct stat statStruct;


		//myFile = fopen(fileName, "rb");
		rc = fopen_s(&myFile, fileName, "rb");
		if (rc)
		{
			printf("Can't open file %s, did you copy it to the right location?\n", fileName);
			exit(97);
		}

		// Get storage space for the disk images
		rc = stat(fileName, &statStruct);
		if (rc)
		{
			printf("Invalid fstat file %s\n", fileName);
			exit(97);
		}

		HDDimage = (uint8_t *)malloc(statStruct.st_size);
		if (HDDimage == NULL)
		{
			printf("Could not malloc %ld bytes\n", statStruct.st_size);
			exit(97);
		}
		

		size = fread(HDDimage, 1, statStruct.st_size, myFile);   //myFile.Read(HDDimage, sizeof(HDDimage));
		fclose(myFile);

		if (size == 0)
		{
			printf("Can't read file %s\n", fileName);
			exit(98);
		}

	
}