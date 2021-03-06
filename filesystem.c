//
//  filesystem.c
//  prog3
//
//  Created by Michael Cole & Justin Guillory on 11/3/17.
//  Copyright © 2017 Michael Cole. All rights reserved.
//  Copyright © 2017 Justin Guillory. All rights reserved.
//
#include "softwaredisk.h"
#include "filesystem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
DirectoryEntry Directory[100];
DirectoryEntry dirBuf;
FATentry FATBuf;
int FATEntriesPerBlock = 512/sizeof(FATentry);
// filesystem error code set (set by each filesystem function)
FSError fserror;

// functions for filesystem API

// open existing file with pathname 'name' and access mode 'mode'.  Current file
// position is set at byte 0.  Returns NULL on error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode)
{
    File fileToOpen;
    //TODO: use the name and mode to either open the file or deny permission
    
     
    
    bzero(&dirBuf,SOFTWARE_DISK_BLOCK_SIZE);
    int ret = read_sd_block(&dirBuf,(unsigned long)fileToOpen->Dir);
    // Scan directory to see if file exists already or not
    if(ret)
    {
	if (dirBuf.isOpen)
	{
	    fserror = 3;
	}
	else 
	{
	    dirBuf.isOpen = 1;
	    fserror = 0;
	}
    }
	   
    else 
	 fserror = 4;
    
    
    return fileToOpen;
};

// create and open new file with pathname 'name' and access mode 'mode'.  Current file
// position is set at byte 0.  Returns NULL on error. Always sets 'fserror' global.
File create_file(char *name, FileMode mode)
{
    fserror = 0;
    //TODO: if error, fileToCreate = NULL
    //
    
    File fileToCreate; //may need to malloc, may need constructor
    fileToCreate = malloc(sizeof(FileInternals));
    fileToCreate->mode = mode;
    fileToCreate->currentPosition = 0;
    //printf("current position = %i\n",fileToCreate->currentPosition);	
    
    // Scan directory to see if file exists already or not
    if(file_exists(name)==1)
	    fserror = 6;
    
    //scan directory for first available entry
    if (fserror == 0)
    {
	for (int i = 0;i < 100; i++)
	{
		bzero(&dirBuf,SOFTWARE_DISK_BLOCK_SIZE);
		int ret = read_sd_block(&dirBuf,(unsigned long)i);
			
		if (dirBuf.Used == 0)
		{
			strcpy(dirBuf.Filename,name);
			fileToCreate->Dir = i;
			dirBuf.Used = 1;
		       	break;	

		}
                if (i==99)
		{
    			//printf("Didn't find spot in directory\n");	
			fserror = 1;	
		}	
	}
    }

    
    if (fserror == 0)
	{
	
	// Looping blocks in FAT  
	for (int j = 100; j < 103; j++)
	    {
	      if (j!= 102)
		{
		  // Looping FAT Entries in 2 blocks
		  for (int z = 0; z < FATEntriesPerBlock; z++)
		    {
			
			bzero(&FATBuf,2*sizeof(int));
			int ret = read_sd_block(&FATBuf,(unsigned long)z);
			if (FATBuf.Used == 0)
			{
					
				FATBuf.Used = 1;
				fileToCreate->FATblock = j;
				fileToCreate->FATblockPosition = z;
				fileToCreate->currentBlock = (100 + (64*(101-j)) + z);
				dirBuf.StartBlock = (100 + (64*(101-j)) + z);
				dirBuf.Used = 1;
				
				//dirBuf.fileRef = fileToCreate;

	      			int ret1 = write_sd_block(&dirBuf,fileToCreate->Dir);
	      			//printf("Return value was %d for Directory block %i \n\n", ret1,fileToCreate->Dir);

	      			int ret2 = write_sd_block(&FATBuf,fileToCreate->FATblock);
	      			//printf("Return value was %d for FAT entry at block %i and position %i\n", ret2,fileToCreate->FATblock,fileToCreate->FATblockPosition);
				//printf("New file located at block %i and FATEntry %i\n", j,z);
				break;
			}
		    }
		}
	      else
		{
		  // Looping last 36 FAT Entries in Block 102
		  for (int z = 0; z < FATEntriesPerBlock - 28; z++)
		    {
			bzero(&FATBuf,2*sizeof(int));
			int ret = read_sd_block(&FATBuf,(unsigned long)z);
			if (FATBuf.Used == 0)
			{
				FATBuf.Used = 1;
				fileToCreate->FATblock = j;
				fileToCreate->FATblockPosition = z;
				fileToCreate->currentBlock = (100 + (64*(101-j)) + z);
				dirBuf.StartBlock = (100 + (64*(101-j)) + z);
				dirBuf.Used = 1;
				//dirBuf.fileRef = fileToCreate;
	      			
				int ret1 = write_sd_block(&dirBuf,fileToCreate->Dir);
	      			//printf("Return value was %d for Directory block %i \n\n", ret1,fileToCreate->Dir);

	      			int ret2 = write_sd_block(&FATBuf,fileToCreate->FATblock);
	      			//printf("Return value was %d for FAT entry at block %i and position %i\n", ret2,fileToCreate->FATblock,fileToCreate->FATblockPosition);
				//printf("New file located at block %i and FATEntry %i\n", j,z);
				break;
			}

			if (z == FATEntriesPerBlock - 27)
				fserror = 1;
		    }
		}
	      

	      if (FATBuf.Used == 1)
			  break;
	    }
	  
	}
    //TODO: handle error, set current file position to 0
//    if (fserror == 0)
//	    open_file(name,mode);implementation of file system in c
    
    return fileToCreate;
};
// close 'file'.  Always sets 'fserror' global.
void close_file(File file)
{
    //TODO: ???
     
    bzero(&dirBuf,SOFTWARE_DISK_BLOCK_SIZE);
    int ret = read_sd_block(&dirBuf,(unsigned long)file->Dir);
    // Scan directory to see if file exists already or not
    if(ret)
    {
	dirBuf.isOpen = 0; 
	fserror = 0;
    }
    else 
	 fserror = 4;
};

// read at most 'numbytes' of data from 'file' into 'buf', starting at the
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes)
{
    unsigned long numReadBytes = 0;
    //TODO: check for EOF
    //TODO: start reading at current position
    //TODO: if ! EOF, numReadBytes ++, read next Position, else return;
    return numReadBytes;
};

// write 'numbytes' of data from 'buf' into 'file' at the current file position.
// Returns the number of bytes written. On an out of space error, the return value may be
// less than 'numbytes'.  Always sets 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes)
{
    unsigned long numBytesWritten = 0;
    //TODO: check if out of space (OOS)
    //TODO: start writing at current position
    //TODO: if ! OOS, numBytesWritten ++, write next position,else return;
    return numBytesWritten;
};

// sets current position in file to 'bytepos', always relative to the beginning of file.
// Seeks past the current end of file should extend the file. Always sets 'fserror'
// global.
void seek_file(File file, unsigned long bytepos)
{
    //TODO: check if bytepos is past end of file
    //TODO: if it is, extend the file
    //TODO: set the current position to bytepos
};

// returns the current length of the file in bytes. Always sets 'fserror' global.
unsigned long file_length(File file)
{
    unsigned long lengthOfFile;
    //TODO: get the length of the file;
    return lengthOfFile;
};

// deletes the file named 'name', if it exists and if the file is closed.
// Fails if the file is currently open. Returns 1 on success, 0 on failure.
// Always sets 'fserror' global.
int delete_file(char *name)
{
    int success = 0;
    //TODO: check if file exists
    //      if not, return 0
    //      if it exists, check if file is open
    //      if it is open, return 0
    //      else, close the file and set success = 1;
    return success;
};

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name)
{
    int exists = 0;
    //scan directory to see if name already exists in directory
    for (int i = 0; i < 100; i++)
    {
	bzero(&dirBuf,SOFTWARE_DISK_BLOCK_SIZE);
	int ret = read_sd_block(&dirBuf,(unsigned long)i);
	if (!strcmp(dirBuf.Filename,name))
	{
		exists = 1;
		break;
	}
    }

    //TODO: check if the file exists

    return exists;
};

// describe current filesystem error code by printing a descriptive message to standard
// error.
void fs_print_error(void)
{
    //printf("FS ERROR: ");
    switch(fserror){
        case FS_NONE:
            puts("NONE");
	    break;
        case FS_OUT_OF_SPACE:
            puts("OUT_OF_SPACE");
	    break;
        case FS_FILE_NOT_OPEN:
            puts("FILE_NOT_OPEN");
	    break;
        case FS_FILE_OPEN:
            puts("FILE_OPEN");
	    break;
        case FS_FILE_NOT_FOUND:
            puts("FILE_NOT_FOUND");
	    break;
        case FS_FILE_READ_ONLY:
            puts("FILE_READ_ONLY");
	    break;
        case FS_FILE_ALREADY_EXISTS:
            puts("FILE_ALREADY_EXISTS");
	    break;
        default: puts("UNKNOWN ERROR");
    }
};
