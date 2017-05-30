/*
 * Implementation of classes to create and manage segments (tarballs).
 * For now, this also maintains the index (though maybe index maintenance
 * should be broken out into its own class).
 */

#include "segment.h"

#include "index.pb.h"
#include "kvfs_mount.h"  // For TEST_FSID.
#include "tiered_in_file.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>  // for strncpy
#include <sys/stat.h>


namespace pqfs {

const bool SHOULD_UPDATE_ATIME = false;

int MkdirPath(const std::string& dir_path);

Segment::Segment(
        const std::string& temp_dir,
        const std::string& tarball_path,
        FileSystemManager* snapshot_fs_manager,
        int64_t fs_id) :
    temp_dir(temp_dir),
    tarball_path(tarball_path),
    snapshot_fs_manager(snapshot_fs_manager),
    fs_id(fs_id),
    committed(false) {
}

// Add a single file or directory to the segment.
// This version creates on-disk copies of the files and directories needed,
// and actually tars them up (via tar command).
// Obviously a production version would write directly to a tarball.
//
// When adding a directory, the directory metadata is stored, but any files
// it may contain are not added.
//
// returns 0 or errno.
int Segment::Add(int64_t inode_num, const std::string& path,
        Segment::ObjectParts parts)
{
    int err = 0;

    VLOG(1) << "Add i " << inode_num << " '" << path<< "'";
    assert(parts == WHOLE_FILE);
    // Stat the file, then copy it out to disk.
    struct stat stat;
    auto status = snapshot_fs_manager->statFile(fs_id, inode_num, &stat);
    if (status != Status::STATUS_OK) {
        // Because the log is not trimmed, we find entries for files that have
        // been deleted since they were modified.
        LOG(INFO) << "Add: Failed, status " << status;
        err = StatusToErrno(status);
    } else if (S_ISREG(stat.st_mode)) {
        // copy the file out to disk, set its attrs based on stat info.
        LOG(INFO) << "Add: copy file i# " << inode_num
            << " path '" << path << "' size " << stat.st_size;
        err = CopyFile(inode_num, path, (off_t)0, stat);
    } else if (S_ISDIR(stat.st_mode)) {
        LOG(INFO) << "Add: dir info i# " << inode_num
            << " '" << path << "' size " << stat.st_size;
        err = CopyDir(inode_num, path, stat);
    } else {
        LOG(ERROR) << "Add: st_mode type " << stat.st_mode << " not supported";
        err = EINVAL;
    }
    if (err == 0) {
        inode_paths[inode_num] = path;
    }
    VLOG(2) << "Segment::Add inode " << inode_num
        << " path '" << path << "' err " << err;
    return err;
}

// Create the actual persistent segment (tarball).
// returns 0 or errno.
int Segment::Commit() {
    // TODO: create incremental backups? Inc.. would create a ".snar" index.
    std::string tar_cmd(
            "tar "
            "--verbose "
            "-z "  // Make a compressed tar file (aka tarball).
            "--create "
            "--file=" + tarball_path + " "
            "--directory " + temp_dir + " "
            ".");  // All files in that temp_dir
    int ret = system(tar_cmd.c_str());
    LOG(INFO) << "Segment::Commit system: '" << tar_cmd << "' ret " << ret;
    if (ret == 0)
        committed = true;
    return ret;
}

int Segment::Close() {
    // TODO: clean up our temp_dir!
    return 0;
}

int Segment::CopyFile(int64_t handle, const std::string& path,
        off_t offset, struct stat& stat) {
    assert(offset == 0);   // non-zero offset NYI
    int file_desc = -1;
    off_t length = stat.st_size - offset;
    uint64_t actual_len = -1;
    char* read_buf = (char*)malloc(length);  // TODO: large files? copy chunks!
    auto status = snapshot_fs_manager->readFile(
            fs_id, handle, offset, length, read_buf, &actual_len,
            SHOULD_UPDATE_ATIME);
    LOG(INFO) << "CopyFile: read status " << status
        << " result " << (int)actual_len << " bytes "
        <<  *(long*)read_buf << "...";
    // Create temp_dir/path
    std::string full_path(temp_dir + "/" + path);

    // Make sure directory path exists.
    std::string dir_path(full_path.substr(0, full_path.find_last_of("/")));
    int ret = MkdirPath(dir_path);
    if (ret != 0) {
        goto out;
    }
    file_desc = open(full_path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, stat.st_mode);
    if (file_desc < 0) {
        perror(full_path.c_str());
        ret = errno;
        goto out;
    }
    if (write(file_desc, read_buf, length) < 0) {
        perror("write");
        ret = errno;
        goto out;
    }
    // write read_buf contents to it.
    free(read_buf);
out:
    if (file_desc >= 0) {
        (void)close(file_desc);
    }
    return ret;
}

int Segment::CopyDir(
        int64_t handle,
        const std::string& path,
        struct stat& stat) {
    // TODO - want to make tar "dirblock" somehow.
    // Create temp_dir/path
    std::string full_path(temp_dir + "/" + path);
    int ret = MkdirPath(full_path);
    // TODO: set modes on dirs.
    // Open the directory (file) for handle and then refactor and use code from
    // FuseKvfsInode::Readdir
    // (Or maybe move the readdir code into fs manager?)
    // auto status = snapshot_fs_manager->statFile(fs_id, inode_num, &stat);
    return ret;
}

// return a TieredInFile which can be used to stat, and read, the
// specified parts of an inode.
TieredInFile*
Segment::OpenFile(const proto::InodeOffset& inode_offset, ObjectParts parts) {
    std::string extracted_file_path(temp_dir + "/" + inode_offset.pathname());
    int file_desc = -1;
    bool did_tar = false;
    // Bodge: check for existence of file before extracting tarball.
    file_desc = open(extracted_file_path.c_str(), O_RDONLY);
    if (file_desc < 0) {
        // TODO: do a more legit on-disk cache, check that size etc match.
        // For now, just re-extract entire tarball for each file.
        // TODO: create incremental backups? Inc.. would create a ".snar" index.
        std::string tar_cmd(
                "tar "
                "--verbose "
                "--extract "  // Make a compressed tar file (aka tarball).
                "--file=" + tarball_path + " "
                "--directory " + temp_dir);
        int ret = system(tar_cmd.c_str());
        did_tar = true;
        DVLOG(2) << "OpenFile system: '" << tar_cmd << "' ret " << ret;
        if (ret != 0) {
            return nullptr;
        }
        file_desc = open(extracted_file_path.c_str(), O_RDONLY);
    }
    DVLOG(1) << "OpenFile did_tar " << did_tar << " open(" <<
        extracted_file_path << ") = " << file_desc;
    if (file_desc < 0) {
        LOG(ERROR) << "OpenFile: " << inode_offset.pathname() << " err " << errno;
        return nullptr;
    }
    return new TieredInFile(file_desc, inode_offset.inode(), inode_offset.pathname());
}

// Close a TieredInFile that was returned by Open.
void CloseFile(TieredInFile* file);

const std::string& Segment::get_name() const {
    return tarball_path;
}

// Returns list of inode numbers in this segment.
const std::map<int64_t, std::string>& Segment::get_inode_paths() const {
    return inode_paths;
}

// Since segments might be implemented all in-memory, or might write files to
// disk first (and then invoke tar), we use a factory for flexibility.
SegmentFactory::SegmentFactory(
        const std::string& working_dir_template,
        CloudProxy* cloud_proxy,
        FileSystemManager* snapshot_fs_manager,
        int64_t fs_id) :
    working_dir_template(working_dir_template + "%ld-%ld"),
    cloud_proxy(cloud_proxy),
    snapshot_fs_manager(snapshot_fs_manager),
    fs_id(fs_id) {
}

Segment*
SegmentFactory::AddSegment(xid_t first_xid, xid_t last_xid) {
    // TODO: Segment should be an abstract class, and this factory exists to
    // construct the appropriate type of segment based on "params".
    std::string working_dir = MakeWorkingDir(first_xid, last_xid);
    if (working_dir.empty()) {
        return nullptr;
    }
    std::ostringstream tarball_path;
    tarball_path << cloud_proxy->get_local_dir()
        << "/tb_" << first_xid << "-" << last_xid
        << ".tgz";   // normal tar gzip file xtension.
    auto* new_segment = new Segment(working_dir, tarball_path.str(),
            snapshot_fs_manager, fs_id);
    new_segment->set_first_xid(first_xid);
    new_segment->set_last_xid(last_xid);
    VLOG(1) << "AddSegment " << first_xid << ".." << last_xid << " dir " <<
        working_dir << " tarball " << tarball_path.str();
    return new_segment;
}

std::string
SegmentFactory::MakeWorkingDir(xid_t first_xid, xid_t last_xid) {
    char working_dir[128];
    size_t num_chars = snprintf(working_dir, sizeof(working_dir),
        working_dir_template.c_str(), first_xid, last_xid);
    if (num_chars >= sizeof(working_dir)) {
        LOG(ERROR) << "SegmentFactory::MakeWorkingDir: working_dir truncated!";
        return "";
    }
    if (MkdirPath(working_dir) != 0) {
        LOG(ERROR) << "AddSegment: MkdirPath('"
            << working_dir << "') failed, errno " << errno;
        return "";
    }
    return working_dir;
}

Segment*
SegmentFactory::OpenSegment(const proto::Segment* proto_segment) {
    LOG(ERROR) << "OpenSegment " << proto_segment->DebugString();
    std::string working_dir = MakeWorkingDir(
        proto_segment->first_xid(), proto_segment->last_xid());
    cloud_proxy->PinFile(proto_segment->name());
    auto* new_segment = new Segment(working_dir, proto_segment->name(),
            nullptr, // no fs_manager needed since won't be adding files
            TEST_FSID);
    return new_segment;
}

void SegmentFactory::DeleteSegment(Segment* segment) {
    // TODO: remove 
    cloud_proxy->UnpinFile(segment->get_name());
}

// Make all the dirs in the path (succeeds even if all exist).
// TODO: don't use system().
int MkdirPath(const std::string& dir_path) {
    std::string mkdir_cmd("mkdir --parents  --verbose '" + dir_path + "'");
    int ret = system(mkdir_cmd.c_str());
    if (ret != 0) {
        LOG(ERROR) << "MkdirPath system(" << mkdir_cmd << ") failed: " << ret;
    }
    return ret;
}

}  // pqfs.
