/*
 * Cloud Proxy: mediates interaction with our immutable objects in the cloud.
 * API to an interface to cloud storage with a persistent local cache.
 * TODO: This should be a richer API, so that the proxy knows when files are
 * created and when they're in use (and which doesn't just sync everything
 * too/from the cloud.
 * TODO: implement limits & pruning of our cache.
 */
#ifndef TIERING_CLOUD_PROXY_H_
#define TIERING_CLOUD_PROXY_H_

#include <string>

namespace pqfs {

class CloudProxy {
public:
    // For now, this just encapsulates a local directory and information about
    // where in the cloud (url, etc) that local dir should be synced to.
    // i.e., allows us to do:
    // s3cmd sync /tmp/proxy_dir/ s3://pqfs-test-0/proxy_dir/
    // Writing to local files is done directly (NEEDSWORK: API).
    CloudProxy(std::string local_dir, std::string s3_base_uri) :
        local_dir(local_dir + "/"),
        s3_base_uri(s3_base_uri + "/") {
    }

    // Make sure appropriate directories exist.
    int Init();

    // TODO: get output stream for a local file within local_dir.
    // ostream* CreateFile(const std::string& path);

    enum SyncDirection {
        SYNC_OUT = 1,  // From local to cloud.
        SYNC_IN = 2    // From cloud to local.
    };

    // Sync between local and the in the specified direction.
    // For now, blocks the calling thread until complete.
    // TODO: Get rid of this big hammer.
    int Sync(SyncDirection direction);

    // Make sure the specified file is present in our local proxy.
    // If not, pull from cloud.
    int PinFile(const std::string& file_path);
    void UnpinFile(const std::string& file_path);

    const std::string LocalPath(const std::string& sub_path) {
        return local_dir + sub_path;
    }

    const std::string CloudUri(const std::string& sub_path) {
        return s3_base_uri + sub_path;
    }

    const std::string& get_local_dir() { return local_dir; }
private:
    std::string local_dir;
    std::string s3_base_uri;
};

} // namespace pqfs
#endif // TIERING_CLOUD_PROXY_H_
