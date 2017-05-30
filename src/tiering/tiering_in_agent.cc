#include "tiering_in_agent.h"

#include "cloud_proxy.h"
#include "index_manager.h"
#include "segment.h"
#include "tiered_in_file.h"

#include <glog/logging.h>
#include <google/protobuf/text_format.h>

#include <assert.h>
#include <stdio.h>
#include <chrono>
#include <thread>

#include <algorithm>
#include <set>
#include <vector>

using google::protobuf::TextFormat;

namespace pqfs {

void Compress(std::vector<proto::ChangeLogEntry>* log_entries);

int TieringInAgent::SetOptions(std::string options) {
    // TODO:
    return 0;
}

int TieringInAgent::RequestStop() {
    // NEEDSWORK: synchronize, notify.
    stop_requested = true;
    return 0;
}

void TieringInAgent::Join() {
    assert(run_thread);
    run_thread->join();
    delete run_thread;
    run_thread = nullptr;
}

#define MAX_XID_PER_SEGMENT  1000

class InodeByPathComparator {
  public:
    InodeByPathComparator(
            std::unordered_map<uint64_t,
            std::string>& paths) :
        paths(paths) {
    }
    bool operator() (uint64_t i, int64_t j) {
        const std::string& path_i = paths[i];
        const std::string& path_j = paths[j];
        return (path_i.compare(path_j) < 0); 
    }
  private:
    std::unordered_map<uint64_t, std::string>& paths;
};

void TieringInAgent::RunInternal() {

    // TODO(): wait on a queue of file requests,
    // for better batching. (For now, we just recover the file on the
    // request thread).

    // Read in current Index, if any.
    do {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        VLOG(4) << "TieringInAgent::RunInternal woke up";
        // 
    } while (!stop_requested);
}

// Main operation:
// OpenFile(inode#, xid);  // latest version after this xid.
//   ask index for segment(s) that might contain this inode.
//   ask proxy cloud proxy for segment
//   ask segment for file handle.
//   stat & read file via handle.
// TODO(higher level): segment API symmetry: Add vs get?
TieredInFile* TieringInAgent::Open(uint64_t inode_num, xid_t min_xid) {
    proto::InodeOffset inode_offset;
    const proto::Segment* proto_segment =
        index_manager->GetSegment(inode_num, min_xid, &inode_offset);
    if (proto_segment == nullptr) {
        LOG(ERROR) << "Open - segment not in index?! ino " << inode_num
                << " min_xid " << min_xid;
        return nullptr;  // TODO: ? what to do here?
    }
    DVLOG(1) << "TieringInAgent::Open i " << inode_num << proto_segment->DebugString();
    // TODO: segment lifetime - should segment_factory manage them?
    Segment* segment = segment_factory->OpenSegment(proto_segment);
    auto* ret_file =
            segment->OpenFile(inode_offset, Segment::ObjectParts::WHOLE_FILE);
    segment->Close();
    delete segment;
    return ret_file;
}

void TieringInAgent::Close(TieredInFile* file) {
    // TODO: Should remove or un-hold any extracted file parts.
    file->Close();
    // file 
    delete file; // TODO: ref count needed?
}

int TieringInAgent::Start() {
    if (run_thread != nullptr) {
        return EINVAL;
    }
    // starts running immediately.
    stop_requested = false;
    run_thread = new std::thread(&TieringInAgent::RunInternal, this);
    return 0;
}

} // namespace pqfs
