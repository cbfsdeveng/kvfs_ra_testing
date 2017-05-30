/*
 * Creates some filesystem test fixtures.
 * (extracted from prasanna's KVFS.cc)
 */
#ifndef FILE_TEST_UTIL_
#define FILE_TEST_UTIL_

class FileSystemManager;

namespace pqfs {

// Required for each brand new filesystem.
void CreateRoot(FileSystemManager* fm, uint64_t fs_id);

#if 0
void CreateDirs(FileSystemManager* fm, uint64_t fs_id);

void WriteOneFile(FileSystemManager* fm, uint64_t fs_id, char* file_name);

void CreateFiles(FileSystemManager* fm, uint64_t fs_id, int num_files);

#endif
void Cleanup(FileSystemManager* fm, uint64_t fs_id);

} // namespace pqfs
#endif  // FILE_TEST_UTIL_
