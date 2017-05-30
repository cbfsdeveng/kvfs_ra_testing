/*
 * Tiered-in-File.
 * Used to access a file from its tarball (which has been pulled back
 * from the cloud).
 */
#ifndef TIERED_IN_FILE_
#define TIERED_IN_FILE_

#include <string>

struct stat;

namespace pqfs {

// TODO: make this abstract, and define TieredInFileImpl
// (for now, this implementation assumes extracted tarballs)
// TODO: move to own file(s).
class TieredInFile {
public:
    TieredInFile(int file_desc, uint64_t inode_num,
            const std::string& path = "");  // path optional for debugging.

    // Read up to size bytes starting at offset into buf.
    // returns number of bytes actualy read.
    size_t Read(char* buf, off_t offset, size_t size);

    int Stat(struct stat* stat, std::string* path = nullptr);

    int Close();

private:
    // Implementation: the file descriptor from which we'll read the file's data.
    // TODO: smarter type, to allow fewer/zero-copy from tarball?
    const int file_desc;
    const uint64_t inode_num;
    const std::string path;
};

}  // namespace pqfs
#endif // TIERED_IN_FILE_
