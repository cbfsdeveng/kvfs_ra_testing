/*
 * Classes to create and manage segments (tarballs).
 */

#ifndef TIERING_SEGMENT_H_
#define TIERING_SEGMENT_H_

#include "FileSystemManager.h"
#include "cloud_proxy.h"
#include "index.pb.h"

#include <set>
#include <string>

namespace pqfs {

class TieredInFile;

// TODO(joe): This should be an abstract class (interface), where this
// file-based-staging version is one implementation.
class Segment {
  public:
    Segment(
        const std::string& temp_dir,  // Staging area for tarball contents.
        const std::string& tarball_path,
        FileSystemManager* snapshot_fs_manager,  // Not owned.
        int64_t fs_id);

    enum ObjectParts {
        WHOLE_FILE=1,
        METADATA_ONLY=2,
        DATA_ONLY=3,
        DATA_RANGE_ONLY=4
    };

    // Add a single file or directory to the segment.
    // When adding a directory, the directory metadata is stored, but any files
    // it may contain are not added.
    // returns 0 or errno.
    int Add(int64_t inode_num, const std::string& path, ObjectParts parts);
    // Create the actual persistent segment (tarball).
    // returns 0 or errno.

    // return a TieredInFile which can be used to stat, and read, the
    // specified parts of an inode.
    TieredInFile* OpenFile(const proto::InodeOffset& inode_offset, ObjectParts parts);
    // Close a TieredInFile that was returned by Open.
    void CloseFile(TieredInFile* file);

    int Commit();

    int Close();

    // setters and getters.
    void set_first_xid(int64_t xid) { first_xid = xid; };
    void set_last_xid(int64_t xid) { last_xid = xid; };
    int64_t get_first_xid() const { return first_xid; };
    int64_t get_last_xid() const { return last_xid; };

    // After Commit(), returns the path of the committed tarball.
    const std::string& get_name() const;

    // Returns ref to set of inode numbers in this segment.
    const std::map<int64_t, std::string>& get_inode_paths() const;

    const int64_t get_fs_id() { return fs_id; }

  private:
    // Copy specified part of an inode to a file path (relative to temp_dir).
    int CopyFile(int64_t handle, const std::string& path,
            off_t offset, struct stat& stat);
    // Create and set attributes for directories in the path.
    int CopyDir(int64_t handle, const std::string& path, struct stat& stat);
    // Full pathname to directory where we put the files and dirs that will
    // become the contents of this segment.
    std::string temp_dir;
    // Pathname where the actual tarball will be written.
    std::string tarball_path;
    // Used to access a filesystem snapshot for backup.
    FileSystemManager* snapshot_fs_manager;  // Not owned
    // The set of inodes that are recorded in this tarball.
    // Info we need to retrieve inode from segment (path, since we don't
    // yet handle offsets in tarballs).
    std::map<int64_t, std::string> inode_paths;
    const int64_t fs_id;
    bool committed;
    // xid range for files in this segment.
    int64_t first_xid;
    int64_t last_xid;
};

// Since segments might be implemented all in-memory, or might write files to
// disk first (and then invoke tar), we use a factory for flexibility.
// NEEDWORK: shouldn't be tied to local storage via paths.
// Pass a key-value map instead.
// TODO: Abstract.
// TODO: PROBABLY NOT A FACTORY. Manages 0 or more Segments, allowing creation,
// (deletion - for GC), open, close.
class SegmentFactory {
  public:
    SegmentFactory(
            const std::string& working_dir_template,
            CloudProxy* cloud_proxy,
            FileSystemManager* snapshot_fs_manager,
            int64_t fs_id);

    Segment* AddSegment(xid_t first_xid, xid_t last_xid);

    // Open a previously created segment.
    // Returned segment is read-only, since segments are immutable.
    Segment* OpenSegment(const proto::Segment*);

    void DeleteSegment(Segment* segment);
  private:
    std::string MakeWorkingDir(xid_t first_xid, xid_t last_xid);

    std::string working_dir_template;
    CloudProxy* cloud_proxy;
    FileSystemManager* snapshot_fs_manager;  // Not owned.
    const int64_t fs_id;
};

}  // pqfs.
#endif  // TIERING_SEGMENT_H_
