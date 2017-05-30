#include <fuse_lowlevel.h>
#include "dir_entry_adder.h"
#include "expect.h"
#include "kvfs_mount.h"

DirEntryAdder::DirEntryAdder(KvfsMount* mount, char* buf, size_t size)
    : mount(mount),
    buf(buf),
    size(size),
    total_bytes_added(0),
    out_of_space(false)
{
}

bool DirEntryAdder::Add(
        const char* name,
        const ino_t ino,
        const off_t next_offset)
{
    FuseKvfsInode* inode = mount->GetInode(ino, true, "DirEntryAdder");
    if (inode == nullptr) {
        LOG(ERROR) << "DirEntryAdder::Add ino " << ino << " not found.";
        return true;  // so caller will continue to his next entry.
    }
    VLOG(1) << "add entry name " << name << " ino " << ino
        << " mode " << inode->attr.st_mode
        << " state " << inode->get_state();
    //EXPECT(inode->get_state() != FuseKvfsInode::State::INVALID);
    EXPECT(inode->attr.st_ino == ino);
    // TODO: put type into dir? Always Refresh here?
    EXPECT(S_ISREG(inode->attr.st_mode) || S_ISDIR(inode->attr.st_mode));
    // Note: attr (only) needs ino and the type part of mode (file/dir/...)
    size_t bytes_needed =
        fuse_add_direntry(
                (fuse_req_t)nullptr,  // TODO - not used.
                buf, size, name, &inode->attr, next_offset);
    inode->Release(1, "DirEntryAdder");
    if (bytes_needed > size) {
        // There was insufficient room for this entry.
        VLOG(2) << "DirEntryAddr::Add: need at least " << bytes_needed
            << " bytes, have "  << size;
        out_of_space = true;
        return false;
    }
    total_bytes_added += bytes_needed;
    buf += bytes_needed;
    size -= bytes_needed;
    return true;
}
