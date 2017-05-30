/*
 * Cloud Proxy: mediates interaction with our objects in the cloud.
 * API to an interface to cloud storage with a persistent local cache.
 */

#include "cloud_proxy.h"

#include "WallTime.h"

#include <glog/logging.h>

namespace pqfs {

int CloudProxy::Init() {
    std::string mkdir_cmd("mkdir --parents  --verbose '" + local_dir + "'");
    int ret = system(mkdir_cmd.c_str());
    LOG(INFO) << "CloudProxy::Init system(" << mkdir_cmd << ") ret " << ret;
    return ret;
}

int CloudProxy::Sync(SyncDirection direction) {
    std::string s3cmd_cmd("rclone -v sync ");
    if (direction == SYNC_OUT) {
        s3cmd_cmd +=  local_dir + " " + s3_base_uri;
    } else if (direction == SYNC_IN) {
        s3cmd_cmd += s3_base_uri  + " " + local_dir;
    }
    time_t start_time = WallTime::secondsTimestampToUnix(
            WallTime::secondsTimestamp());
    int ret = system(s3cmd_cmd.c_str());
    time_t finish_time = WallTime::secondsTimestampToUnix(
            WallTime::secondsTimestamp());
    std::string dir_str(direction == SYNC_IN? "IN" :
            (direction == SYNC_OUT? "OUT" : "???"));
    LOG(INFO) << "CloudProxy::Sync " << dir_str
            << " system(" << s3cmd_cmd << ") elapsed "
            << (finish_time - start_time) << " ret " << ret;
    return ret;
}

// TODO: maintain some kind of pin-count.
int
CloudProxy::PinFile(const std::string& file_path) {
    DVLOG(1) << "PinFile '" << file_path << "'";
    return Sync(SYNC_IN);
}

void CloudProxy::UnpinFile(const std::string& file_path) {
    DVLOG(1) << "UnpinFile '" << file_path << "'";
    // TODO: NYI.
}

} // namespace pqfs
