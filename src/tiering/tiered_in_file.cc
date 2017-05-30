#include "tiered_in_file.h"
#include "segment.h"

#include <assert.h>
#include <chrono>
#include <stdio.h>
#include <sys/stat.h>
#include <thread>

#include <algorithm>
#include <string>

namespace pqfs {

// path is optional - for debugging.
TieredInFile::TieredInFile(int file_desc, uint64_t inode_num,
        const std::string& path)
    : file_desc(file_desc), inode_num(inode_num), path(path) {
    DVLOG(2) << "TieredInFile(" << file_desc << ", '" << path << "')";
}

// Read up to size bytes starting at offset into buf.
// returns number of bytes actualy read.
size_t TieredInFile::Read(char* buf, off_t offset, size_t size) {
    size_t ret;
    ret = pread(file_desc, buf, size, offset);
    DVLOG(2) << "TieredInFile::Read(..., " << size << ") "
        << file_desc << ", '" << path << "'" << " ret " << ret;
    return ret;
}

int TieredInFile::Stat(struct stat* stat, std::string* path) {
    int ret = -1;
    ret = fstat(file_desc, stat);
    DVLOG(2) << "TieredInFile::Stat(...) "
        << file_desc << ", '" << path << "'"
        " ret " << ret << " st_ino " << stat->st_ino;
    if (ret == 0) {
        stat->st_ino = inode_num;
    }
    return ret;
}

int TieredInFile::Close() {
    DVLOG(2) << "TieredInFile::close " << file_desc << " '" << path << "'";
    // TODO: does it make sense for this class to do the close?
    return close(file_desc);
}

} // namespace pqfs
