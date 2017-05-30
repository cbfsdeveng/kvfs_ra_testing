#ifndef KVFS_DIRECTORYPARTITION
#define KVFS_DIRECTORYPARTITION

#include "KVFSObject.h"
#include "dir_entry_adder.h"

#include <city.h>
#ifdef __SSE4_2__
#include "citycrc.h"
#endif

#include <vector>
#include <unordered_map>
#include <map>



/**
 * Cast a bigger int down to a smaller one.
 * Asserts that no precision is lost at runtime.
 */
template<typename Small, typename Large>
Small
downCast(const Large& large)
{
    Small small = static_cast<Small>(large);
    // The following comparison (rather than "large==small") allows
    // this method to convert between signed and unsigned values.
    //assert(large-small == 0);
    return small;
}

#define sizeof32(type) downCast<uint32_t>(sizeof(type))

class LogFSDirectoryPartition {
public:
    LogFSDirectoryPartition(uint16_t partitionNumber,
                          uint64_t fsId,
                          uint64_t dir_ino,
                          KVStore* kvstore):
                            partitionNumber(partitionNumber),
                            fsId(fsId),
                            dir_ino(dir_ino),
                            entries(),
                            entries_by_name(),
                            state(INVALID),
                            kvstore(kvstore){}


    LogFSDirectoryPartition& operator = (
            const LogFSDirectoryPartition& partition)
    {
        this->partitionNumber = partition.partitionNumber;
        this->fsId = partition.fsId;
        this->dir_ino  = partition.dir_ino;
        this->kvstore = partition.kvstore;
        this->entries = partition.entries;
        this->entries_by_name = partition.entries_by_name;
        return *this;
    }

    ~LogFSDirectoryPartition() { }

    // If current state is INVALID, read our contents from KVS and deserialize
    // into this->entries*.
    // Returns an errno.
    int Refresh();

    // Returns errno.
    int insertEntry(uint64_t handle, const std::string& name);

    // Returns errno.
    int removeEntry(uint64_t handle, const std::string& name);

    // First entry  added will have offset cur_offset or greater.
    // Offsets are tokens passed to fuse_add_... which are then sent back to us on
    // subsequent readdir calls.
    int listEntries(
            DirEntryAdder* dir_entry_adder,
            off_t cur_offset,         // First offset desired.
            off_t part_base_offset);  // first offset in our partition.

    bool Deserialize(std::string& outBuffer);

    bool isPartitionEmpty() ;

    uint64_t findEntry(const std::string& name);

    Status
    readFromLog(std::string& outBuffer);

    Status
    writeToLog();


    inline uint16_t getPartitionNumber() {
      return partitionNumber;
    }

    inline uint64_t getFSId() {
      return fsId;
    }

    inline uint64_t get_dir_ino() {
      return dir_ino;
    }

private:
    uint16_t partitionNumber;
    uint64_t fsId;
    uint64_t dir_ino;

    // We keep directory entries in a vector so we can index into them during
    // repeated readdirs.
    std::vector<std::pair<std::string, uint64_t>> entries;  // OWNED
    // Ordered map - ordered on name, so that dirs are sorted by name, making 
    // TODO: don't keep a separate copy of string here - point at entries[].
    std::map<std::string, uint64_t> entries_by_name;  // OWNED
    enum State {
        INVALID,
        CLEAN,
        DIRTY
    } state;
    // TODO: remove this.
    // std::unordered_map<uint64_t, std::string> entriesIndexByHandle;

    KVStore* kvstore;

    void Serialize(std::string& buffer);
};

#endif
