#ifndef _DIR_ENTRY_ADDER_
#define _DIR_ENTRY_ADDER_

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class KvfsMount;

class DirEntryAdder {
public:
    // TODO: Should this also handle offsets?
    DirEntryAdder(
            KvfsMount* mount,
            char* buf,
            size_t size);

    // Returns true if added, false if out-of-space.
    bool Add(
        const char* name,
        const ino_t ino,
        const off_t next_offset);
    const bool get_out_of_space() { return out_of_space; }
    const size_t get_total_bytes_added() { return total_bytes_added; }
private:
    KvfsMount* mount;
    char* buf;
    size_t size;
    size_t total_bytes_added;
    bool out_of_space;
};
#endif  //  _DIR_ENTRY_ADDER_
