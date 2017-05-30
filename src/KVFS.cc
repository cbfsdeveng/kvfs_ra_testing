/*
 * TestKVStoreFactory.cc
 *
 *  Created on: Oct 12, 2015
 *      Author: Prasanna Ponnada
 */
#include <iostream>
/*
#include <boost/serialization/export.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
*/

#include "FileSystemManager.h"
#include "KVStoreFactory.h"
#include "LevelDBFactory.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     //to generate random number


void printDirrectory(char *buffPtr, uint64_t count)
{
    uint64_t entryHandle; 
    const char *name;
    uint32_t nameLen;
    
    std::cout << "Listing directories in /:" << std::endl;
    while(count > 0) {
        std::string nameString;
        entryHandle = *reinterpret_cast<const uint64_t*>(buffPtr);
    
        name = buffPtr + sizeof(uint64_t);
        nameLen = (uint32_t)(strchr(name, '|') - name);
        nameString.append(name, nameLen);
    
        buffPtr = buffPtr + sizeof(uint64_t) + nameLen + 1;
        count = count - sizeof(uint64_t) - nameLen - 1;
    
        std::cout << "  " << nameString <<": " << entryHandle << std::endl;
    }  
}

void readEnumeration(FileSystemManager* fm)
{
    uint64_t count; 
    uint32_t offset, length, fileSize, expectedLen;
    uint64_t handle;
   
    /*prepare test data, len = 64*/
    char initial[] = 
        "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-"; 
    char chunk0[]  = 
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-abcdefghijklmnopqrstuvwxyz";  
    char chunk1[]  = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ-abcdefghijklmnopqrstuvwxyz0123456789";  
    char chunk2[]  = 
        "zyxwvutsrqponmlkjihgfedcba9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA#";  
    char chunk3[]  = 
        "9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA#zyxwvutsrqponmlkjihgfedcba";  
    

    char*  writeBuffer = new char[64*5];
    char* testdata = writeBuffer;

    memcpy(writeBuffer, initial, 64);
    memcpy(writeBuffer + 64, chunk0, 64);
    memcpy(writeBuffer + 64*2, chunk1, 64);
    memcpy(writeBuffer + 64*3, chunk2, 64);
    memcpy(writeBuffer + 64*4, chunk3, 64);    

    /*add file*/
    if (fm->createFile(0, 1, "testfile", handle, 0755, 0, 64) == Status::STATUS_OK)
    {
        std::cout << "Created file: /testfile, handle = " << handle <<std::endl;
    }
   
    /*write to the file: initial part and chunk part*/  
    //note: here DEFAULT_FILE_INITIAL_SIZE = 64 for easy debug, 
    //remember to add parameter to createFile
    count = 0;
    if (fm->writeFile(0, handle, 0, 320, writeBuffer, &count) != Status::STATUS_OK)
    {
        std::cout << "Write file fail" <<std::endl;
    }

    if (count != 320)
        std::cout << "Write file length fail" <<std::endl;   


    /*read from the  file: vary offset and length to enumerate all options */
    fileSize = 320;
    char*  readResult = new char[512];
    for(offset = 0; offset < fileSize; offset++) {

        for(length = 0; length < fileSize; length++) {

            if (fm->readFile(0, handle, offset, length, readResult, &count, false) != Status::STATUS_OK)
            {
                std::cout << "Read file fail" <<std::endl;
            }

            //check returned length
            expectedLen = length;
            if((offset + length) > fileSize) {
                expectedLen = fileSize - offset;
            }
            if(count != expectedLen)
                std::cout << "Read file length fail" <<std::endl; 

            //check returned data
            for(uint32_t i = 0; i < (uint32_t)count; i++) {
                if(testdata[offset + i] != readResult[i])
                    std::cout << "Read data fail" <<std::endl;
            }        
        }       

    }   
    delete [] readResult;
    delete [] writeBuffer;

    /*remove file*/
    if (fm->removeFile(0, 1, "testfile") != Status::STATUS_OK)
    {
        std::cout << "Delete testfile fail" <<std::endl;
    }     
    else
        std::cout << "Delete testfile succeed" <<std::endl;
}

void writeEnumeration(FileSystemManager* fm)
{
    uint64_t count = 0;
    uint64_t handle;
    uint32_t offset, length, fileSize, expectedLen;
    
    Status status;
    
    //len = 64
    char pool[] = 
       "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-";  
    
    char testdata[64*5] = {0}; //testdata is used for generating and comparing data     
    fileSize = 320;  //total (1+ 4) = 5 chunk  

    /*add file*/
    if (fm->createFile(0, 1, "testfile", handle, 0755, 0, 64) == Status::STATUS_OK)
    {
        std::cout << "Created file: /testfile, handle = " << handle <<std::endl;
    }
    
    /*write to the file*/
    char*  writeBuffer = new char[512];
    char*  readResult = new char[512];
    srand(0);
    for(offset = 0; offset < fileSize; offset++) {
    
        for(length = 1; length <= (fileSize - offset); length++) {
    
            //generate new pattern data
            for(uint32_t i = 0; i < length; i++) {
                testdata[offset + i] = pool[rand()&63];
            }
    
            //write the test range to file
            memcpy(writeBuffer, &testdata[offset], length);
    
            status = fm->writeFile(0, handle, offset, length, writeBuffer, &count);
            if(status != Status::STATUS_OK) {
                std::cout << "Write fail" <<std::endl;
            }   

            if(count != length)
                std::cout << "Write length fail" <<std::endl;
    
            //read and verify the entire file
            status = fm->readFile(0, handle, 0, fileSize, readResult, &count, false);
            if(status != Status::STATUS_OK) {
                std::cout << "Read fail" <<std::endl;
            }           
    
            //check returned length
            expectedLen = (offset == 0) ? length:fileSize;
            if(count != expectedLen)
                std::cout << "Write data length fail" <<std::endl;
    
            //check returned data
            for(uint32_t i = 0; i < (uint32_t)count; i++) {
    
               if(testdata[i] != readResult[i])
                   std::cout << "Write data fail" <<std::endl;
            }          
    
        }
    
    }
    delete [] readResult;
    delete [] writeBuffer;
    
    
    /*remove file*/
    if (fm->removeFile(0, 1, "testfile") != Status::STATUS_OK)
    {
        std::cout << "Delete testfile fail" <<std::endl;
    }     
    else
        std::cout << "Delete testfile succeed" <<std::endl;

}


void fileExtension(FileSystemManager* fm)
{
     uint64_t count = 0;
     uint64_t handle;
     uint32_t offset, length, initialFileSize, expectedLen, chunksize;

     char pool[] = 
        "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-"; 

     //testdata is used for generating and comparing data, 
     //and 16384 is enough for chunksize = 64
     char testdata[16384] = {0}; 

     chunksize = 64;
     initialFileSize = 320;  //total (1+ 4) = 5 chunk

     /*add file*/
     if (fm->createFile(0, 1, "testfile", handle, 0755, 0, 64) == Status::STATUS_OK)
     {
         std::cout << "Created file: /testfile, handle = " << handle <<std::endl;
     }

     /*initialize the file with 0*/
     char*  writeBuffer = new char[16384];
     memset(writeBuffer, 0, 16384);

     if(fm->writeFile(0, handle, 0, initialFileSize, 
                                     writeBuffer, &count) != Status::STATUS_OK)
     {
         std::cout << "Write file fail" <<std::endl;
     }
     if(count != initialFileSize)
     {
         std::cout << "Write length fail" <<std::endl;
     }

     //extend the buffer
     char*  readResult = new char[16384];
     srand(0);
     offset = (7*chunksize/2);
     length = 1;
     while(length <= 2*chunksize) {
        
         //generate len new bytes at offset
         for(uint32_t i = 0; i < length; i++) {
             testdata[offset + i] = pool[rand()&63];
         }
         
         //write len bytes at offset
         memcpy(writeBuffer, &testdata[offset], length);
         
         if(fm->writeFile(0, handle, offset, length, 
                                    writeBuffer, &count) != Status::STATUS_OK)
         {
             std::cout << "Write file fail" <<std::endl;
         }
         if(count != length)
         {
             std::cout << "Write length fail" <<std::endl;
         }

         //verify length, read and verify entire file, verify read past EOF returns nothing
         expectedLen = ((offset + length) > initialFileSize) ?  
                                             (offset + length): initialFileSize;     
         if(fm->readFile(0, handle, 0, expectedLen, 
                                     readResult, &count, false) != Status::STATUS_OK)
         {
             std::cout << "Read file fail" <<std::endl;
         }
         
         if(count != expectedLen)
         {
             std::cout << "Read length fail" <<std::endl;
         }

         //check returned data
         for(uint32_t i = 0; i < (uint32_t)count; i++) {

            if(testdata[i] != readResult[i])
                std::cout << "Write data fail" <<std::endl;
         }           
         
         offset += length;
         length += 1;
    }
    delete [] readResult;
    delete [] writeBuffer;

     /*remove file*/
     if (fm->removeFile(0, 1, "testfile") != Status::STATUS_OK)
     {
         std::cout << "Delete testfile fail" <<std::endl;
     }     
     else
         std::cout << "Delete testfile succeed" <<std::endl;


}

#undef LOCAL_HD
#define LOCAL_SSD

int main() {
	std::string coordinatorLocation;
	coordinatorLocation = "www.coordinator.com";
#ifdef LOCAL_HD
	KVStoreFactory* factory = new LevelDBFactory;
	KVStore* store = factory->createLocalHDInstance();
	FileSystemManager* fm = new FileSystemManager(store);
#elif LOCAL_SSD
	KVStoreFactory* factory = new RocksDBFactory;
	KVStore* store = factory->createLocalSSDInstance();
#elif REMOTE
	KVStoreFactory* factory = new RamCloudFactory;
	KVStore* store = factory->createRemoteInstance();
#endif
	if (store != nullptr)
		store->init(coordinatorLocation, true);

	std::string fsName = "mykvfs.db";

        auto* change_log = new pqfs::FsChangeLog("", 0);
        change_log->Init();
	if (!fm->mountFileSystem(fsName, change_log))
		std::cout << "Could not create file system: " << fsName <<
		std::endl;
	else
		std::cout << "Mounted file system: " << fsName << std::endl;
    ////////////////////////////////////////////////////////////////////////////

    uint64_t handle;
    char buff[1024];
    uint64_t count = 0;

    /*********************Directory create/list/delete************************/
    std::cout << "**************************************************************" << std::endl;
    std::cout << "Directory create/list/delete test" << std::endl;
    
	if (fm->addDirectory(0, 0, "/", 0, handle) == Status::STATUS_OK)
		std::cout << "Created directory: /, handle =" << handle << std::endl;
	else
		std::cerr << "Failed to create directory: /" << std::endl;

	if (fm->addDirectory(0, 1, "home", 0, handle) == Status::STATUS_OK)
		std::cout << "Created directory: /home, handle = " << handle <<std::endl;
	else
		std::cerr << "Failed to create directory: /home" << std::endl;

	if (fm->addDirectory(0, 1, "tmp", 0, handle) == Status::STATUS_OK)
		std::cout << "Created directory: /tmp, handle = " << handle << std::endl;
	else
		std::cerr << "Failed to create directory: /tmp" << std::endl;

	std::cout << "Trying to create an existing directory: /home" << std::endl;
	if (fm->addDirectory(0, 1, "home", 0, handle) != Status::STATUS_OK)
		std::cout << "Failed to create existing directory: /home" << std::endl;

	if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
	}

    if (fm->removeDirectory(0, 1, "tmp") == Status::STATUS_OK) {
        std::cout << "Delete directory: /tmp succeed" << std::endl;
    }
    else
        std::cout << "Delete directory: /tmp fail" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
	}

    // Dump KVS before umount (which closes KVS connection)
    store->dump(std::cout);

    if (fm->removeDirectory(0, 1, "home") == Status::STATUS_OK) {
        std::cout << "Delete directory: /home succeed" << std::endl;
    }
    else
        std::cout << "Delete directory: /home fail" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
	}

    /*********************File create/list/delete************************/
    std::cout << "**************************************************************" << std::endl;
    std::cout << "File create/list/delete test" << std::endl;   
    
    if (fm->createFile(0, 1, "testfile1", handle, 0755, 0, 64) == Status::STATUS_OK)
        std::cout << "Created file: /testfile1, handle = " << handle <<std::endl;
    else
        std::cerr << "Failed to create file: /testfile1" << std::endl;

    if (fm->createFile(0, 1, "testfile2", handle, 0755, 0, 64) == Status::STATUS_OK)
        std::cout << "Created file: /testfile2, handle = " << handle <<std::endl;
    else
        std::cerr << "Failed to create file: /testfile2" << std::endl;    

    std::cout << "Trying to create an existing file: /testfile1" << std::endl;
    if (fm->createFile(0, 1, "testfile1", handle, 0755, 0, 64) != Status::STATUS_OK)
        std::cout << "Failed to create existing file: /testfile1" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
	}

    if (fm->removeFile(0, 1, "testfile1") == Status::STATUS_OK) {
        std::cout << "Delete file: /testfile1 succeed" << std::endl;
    }
    else
        std::cout << "Delete file: /testfile1 fail" << std::endl;
        
    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
    }

    if (fm->removeFile(0, 1, "testfile2") == Status::STATUS_OK) {
        std::cout << "Delete file: /testfile2 succeed" << std::endl;
    }
    else
        std::cout << "Delete file: /testfile2 fail" << std::endl;
        
    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
    }

    /************************File write/read*******************************/
    std::cout << "**************************************************************" << std::endl;
    std::cout << "File write/read test" << std::endl;  
    
    std::cout << "Begin readEnumeration Test" << std::endl;
    readEnumeration(fm);
    std::cout << "Finish readEnumeration Test" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
    }

    std::cout << "Begin writeEnumeration Test" << std::endl;
    writeEnumeration(fm);
    std::cout << "Finish writeEnumeration Test" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
    }
    
    std::cout << "Begin fileExtension Test" << std::endl;
    fileExtension(fm);
    std::cout << "Finish fileExtension Test" << std::endl;

    memset(buff, 0, 1024);
    if (fm->listDirectory(0, 1, buff, 1024, &count)== Status::STATUS_OK ) {
        printDirrectory(buff, count);         
    }
    store->dump(std::cout);
    
    ////////////////////////////////////////////////////////////////////////////
    std::cout << "**************************************************************" << std::endl;
    fm->umountFileSystem();
    std::cout << "Umount FileSystem" << std::endl;


    //delete db for test
    if(store->destroy(fsName))
        std::cout << "Destroy DB successfully" << std::endl;

    else
        std::cout << "Destroy DB failed" << std::endl;

    //delete all new object
    delete fm;
    delete store;
    delete factory;
        
	return 0;
}

