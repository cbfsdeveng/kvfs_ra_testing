/*
 * Creates some filesystem test fixtures.
 * (extracted from prasanna's KVFS.cc)
 */
#include <iostream>

#include "FileSystemManager.h"
#include "LevelDBFactory.h"
#include "expect.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     //to generate random number

#include "file_test_util.h"
#include "fs_change_log.h"

namespace pqfs {

const uint64_t ROOT_HANDLE = FileSystemManager::ROOT_HANDLE;
uint64_t tmp_handle = 0;  // Set to handle of "/tmp" when it's created.

#if 0
void WriteOneFile(FileSystemManager* fm,
        uint64_t fs_id,
        uint64_t parent_handle,
        std::string& file_name) {
    uint64_t count;
    uint64_t handle;

    /* add file*/
    EXPECT(fm->createFile(fs_id, parent_handle, file_name, handle, 0755, 0, 64)
            == Status::STATUS_OK);
    /*write to the file: initial part and chunk part*/
    //note: here DEFAULT_FILE_INITIAL_SIZE = 64 for easy debug,
    //remember to add parameter to createFile

    /*prepare test data, len = 64*/
    static const char writeBuffer[320] =
        "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-"
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ-abcdefghijklmnopqrstuvwxyz0123456789"
        "zyxwvutsrqponmlkjihgfedcba9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA#"
        "9876543210ZYXWVUTSRQPONMLKJIHGFEDCBA#zyxwvutsrqponmlkjihgfedcba";
    count = 0;
    EXPECT(fm->writeFile(fs_id, handle, 0, sizeof(writeBuffer),
                writeBuffer, &count) == Status::STATUS_OK);
    EXPECT(count == sizeof(writeBuffer));
}
#endif

void CreateRoot(FileSystemManager* fm, uint64_t fs_id) {
    LOG(INFO) << "CreateRoot:" <<
        fm->createRoot(fs_id, 0777);
}

#if 0
void CreateDirs(FileSystemManager* fm, uint64_t fs_id) {
    uint64_t handle;
    fm->addDirectory(fs_id, ROOT_HANDLE, "home", 0755, handle);
    LOG(INFO) << "/home handle " << handle;
    fm->addDirectory(fs_id, ROOT_HANDLE, "tmp", 0755, handle);
    LOG(INFO) << "/tmp handle " << handle;
    tmp_handle = handle;
}
#endif

#if 0
void CreateFiles(FileSystemManager* fm, uint64_t fs_id, int num_files) {
    for (int i = 1; i <= num_files; i++) {
        char strbuf[64];
        snprintf(strbuf, sizeof strbuf, "%s%05d", "test", i);
        std::string file_name(strbuf);
        WriteOneFile(fm, fs_id, tmp_handle, file_name);
    }
}
#endif

void Cleanup(FileSystemManager* fm, uint64_t fs_id) {
#if 0
        fm->removeDirectory(fs_id, ROOT_HANDLE, "tmp");
        fm->removeDirectory(fs_id, ROOT_HANDLE, "home");
        fm->removeFile(fs_id, ROOT_HANDLE, "testfile");
        fm->removeFile(fs_id, ROOT_HANDLE, "testfile1");
        fm->removeFile(fs_id, ROOT_HANDLE, "testfile2");
#endif
}

} // namespace pqfs
