# FAT32-demo
Simple, generic FAT 32 SD card read,not using any MS libraries.  
This was written as a test tool for a Raspberry Pie (RPI) bare metal FAT read program
A sample FAT32 image is provided for testing.
The disk is simulated from a RAW image copy of a real FAT32 disk.

# Usage
FAT32-demo  DiskImage.raw FileToFind

Where: DiskImage.raw is a raw image of a FAT32 formatted disk.
FileToFind    is the file to be located and read.  There are 3 pre-installed files in the sample RAW file: ONE.TXT, TWOTWO.TXT, BONFIRE.PNG
e.g. FAT32-demo fat32-64mb.raw TWOTWO.TXT
