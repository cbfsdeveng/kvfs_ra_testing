#include "KVFSDirectoryPartition.h"
#include <assert.h>
#include "dir_entry_adder.h"
#include "expect.h"
#include <fuse_lowlevel.h>
#include <glog/logging.h>
#include <stdio.h>
#include <string.h>

int
LogFSDirectoryPartition::insertEntry(uint64_t ino,
                                       const std::string& name)
{
    EXPECT(Refresh() == 0);
    EXPECT(state == DIRTY || state == CLEAN);
    if (entries_by_name.count(name) != 0) {
        return EEXIST;
    }
    // TODO: DRY - this duplicates code in Deserialize.
    entries.emplace_back(name, ino);
    entries_by_name[name] = ino;
    state = DIRTY;
    writeToLog(); // TODO: defer.
    return 0;
}

int
LogFSDirectoryPartition::listEntries(
        DirEntryAdder* dir_entry_adder,
        off_t cur_offset,       // First offset desired.
        off_t part_base_offset) // offset of the first entry in our partition.
{
    int err = Refresh();
    if (err != 0) {
        LOG(ERROR) << "listEntries: Refresh err " << err;
        return err;
    }
    VLOG(2) << "listEntries part " << this->partitionNumber << " cur off " <<
        cur_offset << " part_base_off " << part_base_offset;
    EXPECT(cur_offset >= part_base_offset);
    size_t i = cur_offset - part_base_offset;
    for ( ; i < entries.size(); i++) {
        auto& entry = entries[i];
        std::string& name = entry.first;
        uint64_t ino = entry.second;
        auto entry_by_name = entries_by_name.find(name);
        if (entry_by_name == entries_by_name.end()
                || entry_by_name->second != ino) {
            // unlinked entries are removed from entries_by_name, but not from
            // entries (to avoid linear search).
            // TODO: Should we empty the entry here so won't be found again?
            // TODO: does this need generation numbers for the
            // rename-back-to-old-name case?
            VLOG(3) << "listEntries: skip deleted entry name " << name
                << " ino " << ino;
            continue;
        }
        // Current entry is at offset part_base_offset+i.
        off_t next_offset = part_base_offset + i + 1;
        if (!dir_entry_adder->Add(name.c_str(), ino, next_offset)) {
            break;
        }
    }
    return 0;
}

bool
LogFSDirectoryPartition::isPartitionEmpty()
{
    EXPECT(Refresh() == 0);
    if (entries_by_name.size() > 2) {
        return false;
    }
    for (auto e : entries_by_name) {
        if (e.first != "." && e.first != "..") {
            return false;
        }
    }
    return true;
}

uint64_t
LogFSDirectoryPartition::findEntry(const std::string& name)
{
    EXPECT(Refresh() == 0);
    auto entry = entries_by_name.find(name);
    if (entry == entries_by_name.end()) {
        return 0;
    }
    return entry->second;
}

int
LogFSDirectoryPartition::removeEntry(uint64_t handle, const std::string& name)
{
    int err = 0;
    if ((err = Refresh()) == 0) {
        auto entry = entries_by_name.find(name);
        if (entry == entries_by_name.end()) {
            err = ENOENT;
        } else {
            entries_by_name.erase(entry);
            state = DIRTY;
            writeToLog(); // TODO: defer.
        }
    }
    return err;
}

// If necessary, read the directory partition from KVS and build stash in
// this->entries*
int LogFSDirectoryPartition::Refresh() {
    if (state == INVALID) {
        std::string buf;
        Status status = this->readFromLog(buf);
        if (status == Status::STATUS_OK) {
            if (!Deserialize(buf)) {
                LOG(ERROR) << "Refresh Deserialize failed ino " << dir_ino;
                return EIO;
            }
        } else {
            // Partition doesn't exist yet, so our empty state is valid.
            entries_by_name.clear();
        }
        state = CLEAN;
    }
    return 0;
}

// TODO: use actual null, since std::string handles nulls fine.
static const char null_termination = '|';

bool
LogFSDirectoryPartition::Deserialize(std::string& outBuffer)
{
    if (outBuffer.size() < (sizeof(partitionNumber)
                         + sizeof(fsId) + sizeof(dir_ino))) {
        return false;
    }
    uint32_t offset = 0;
    const void* buffer = (const void*)outBuffer.c_str();

    // TODO: a simple struct overlay would be better than these casts.
    fsId = *reinterpret_cast<const uint64_t*>(buffer);
    uint64_t ino = *reinterpret_cast<const uint64_t*>(
                   reinterpret_cast<const uint8_t*>(buffer) + sizeof(uint64_t));
    // We were constructed with dir_ino, so better match.
    EXPECT(this->dir_ino == ino);
    assert(this->dir_ino == ino);
    this->dir_ino = ino;
    partitionNumber = *reinterpret_cast<const uint16_t*>(
                     reinterpret_cast<const uint8_t*>(buffer) + sizeof(uint64_t)
                    + sizeof(uint64_t));

    offset = (uint32_t)(sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint16_t));
    //check if match
    if (    this->fsId != fsId
         || this->partitionNumber != partitionNumber
         || this->dir_ino != ino ) {
        LOG(ERROR) << "Deserialize mismatch!";
        return false;
    }
    entries_by_name.clear();  // TODO: is this the right place to clear?
    uint32_t totalSize = outBuffer.size() - offset;
    if(totalSize > 0) {
        const char *name;
        uint32_t nameLen;
        const char *buff = reinterpret_cast<const char *>(
                     reinterpret_cast<const uint8_t*>(buffer) + offset);
        while (totalSize > 0) {
            ino = *reinterpret_cast<const uint64_t*>(buff);
            name = buff + sizeof(uint64_t);
            nameLen = (uint32_t)(strchr(name, null_termination) - name);
            std::string name_str(name, nameLen);
            entries.emplace_back(name_str, ino);
            // TODO: refer to entries[] instead having copy of string here.
            entries_by_name[name_str] = ino;
            // entriesIndexByHandle[ino].append(name, nameLen);
            buff += sizeof32(uint64_t) + nameLen + 1;
            totalSize -= sizeof32(uint64_t) + nameLen + 1;
        }
    }
    state = CLEAN;
    VLOG(1) << "Deserialize entries.size " << entries.size()
        << " map.size " << entries_by_name.size();
    return true;
}

Status
LogFSDirectoryPartition::readFromLog(std::string& outBuffer) {
    std::string partitionKey;
    //the key format will be changed by fsid+handle+partitionIndex: to be done
    char key[128];
    snprintf(key, 128, "%lu.%d", dir_ino, partitionNumber);
    if(kvstore->get(key, outBuffer))
        return Status::STATUS_OK;
    else
        return Status::STATUS_OBJECT_DOESNT_EXIST;
}

void
LogFSDirectoryPartition::Serialize(std::string& buffer) {
    buffer.append(reinterpret_cast<char *>(&fsId), sizeof(fsId));
    buffer.append(reinterpret_cast<char *>(&dir_ino), sizeof(dir_ino));
    buffer.append(reinterpret_cast<char *>(&partitionNumber), sizeof(partitionNumber));
    // TODO: writes the {inode, string} entries separated by an
    // unstuffed pipe-char. No reason not to use actual nulls, right?
    for (auto it = entries_by_name.begin(); it != entries_by_name.end(); it++) {
        std::string name = it->first;
        uint64_t ino = it->second;
        buffer.append((char *)(&ino), sizeof(ino_t));
        // TODO: Only need 1: string lenght or termination.
        buffer.append((char *)(name.c_str()), (uint32_t)(name.size()));
        buffer.append(&null_termination, 1);
    }
}

Status
LogFSDirectoryPartition::writeToLog() {
    if (state != DIRTY) {
        LOG(WARNING) << "writeToLog: !DIRTY";
        return Status::STATUS_INTERNAL_ERROR;
    }
    char key[128];
    snprintf(key, 128, "%lu.%d", dir_ino, partitionNumber);

    std::string fsDirPartitionBuffer;
    // TODO: readFromLog doesn't deserialize, so this shouldn't serialize (or we
    // should be called "Flush", not writeToLog).
    Serialize(fsDirPartitionBuffer);

    if(kvstore->put(key,fsDirPartitionBuffer)) {
        state = CLEAN;
        return Status::STATUS_OK;
    } else {
        LOG(ERROR) << "LogFSDirectoryPartition::writeToLog put failed key"
            << key;
        return Status::STATUS_INTERNAL_ERROR;
    }
}
