// FAT32-demo.cpp : This file contains the 'main' function. Program execution begins and ends there.
// ------------------------------------------------------------------------ -
// main.c
// This program uses serial communication to display a FAT entry
// 07/07/2013 - Initial version
// 10/14/2013 - set head/tail =0;
// 11/04/2013 - Made initHDD() void, added decimal converter code
// 11/17/2013 - Removed FAT.H
//-------------------------------------------------------------------------
//
// How to set runtime options
// project -> FAT32 demo properties -> general properties -> debugging 
//-------------------------------------------------------------------------


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmpe240.h"

/*-------------------------------------------------------------------------
   Use this for your fatRead() IO read buffer
-------------------------------------------------------------------------*/
uint8_t readBuffer[32 * 1024 * 1024];


/*---------------------------------------------------------------------------
  The main entry point
---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    // Test file
    //const char* testFile = "ONE.TXT";
    //const char* testFile = "TWOTWO.TXT";
    const char* testFile = "BONFIR~1.PNG";

    // Stores the read file handle
    FileHandle fileHandleStorage;

    //-----------------------------------------------------------------------
    //   General looping variable
    //-----------------------------------------------------------------------
    uint32_t  i;
    unsigned int counter;

    if (argc != 2) {
        printf("Error: please provide the name of the disk image files\n");
        printf("FAT32-demo  FAT_IMG_FILE\n");
        exit(99);
    }


    printf("Reading image file '%s'\n", argv[1]);

    /*-------------------------------------------------------------------------
      Check data structure sizes
      -------------------------------------------------------------------------*/
    if (sizeof(partitionDescription) != 16)
    {
        printf("Error: partitionDescription is the wrong size\n");
        return(99);
    }

    if (sizeof(bootRecord) != 512)
    {
        printf("Error: bootRecord is the wrong size\n");
        return(99);
    }

    if (sizeof(FAT_VolID) != 512)
    {
        printf("Error: FAT_VolID is the wrong size\n");
        return(99);
    }

    if (sizeof(FAT_DirEntry) != 32) 
    {
        printf("Error: FAT_DirEntry is the wrong size\n");
        return(99);
    }

    if (sizeof(FAT_DirSector) != 512)
    {
        printf("Error: FAT_DirSector is the wrong size\n");
        return(99);
    }

    readHDDimg(argv[1]);

    // Read the boot sector
    initHDD();


    if (!fatOpen(testFile, &fileHandleStorage))
    {
        printf("Error could not find %s\n\n", testFile);
    }


    // Display debug information
    printf("%s file size= ", testFile);
    printf("%0xX ", fileHandleStorage.FileSize);
    printf("attr= ");
    printf("%0xX ", fileHandleStorage.Attr);
    printf("cluster= ");
    printf("%0xX ", fileHandleStorage.startingCluster);


    // Dump the contents of the file
    counter = fatRead(&fileHandleStorage, &readBuffer[0], sizeof(readBuffer));

    printf("File contents: first 100 bytes---------------\n");
    for (i = 0; i < 100; i++)
    {
        printf("%c",readBuffer[i]);
    }

    // printf("Enable processor interrupts\r\n");
        
    printf("Done\r\n");
   
    return(0);
} // End main
