/*
 * Manage config file reading.
 */

#include "config_manager.h"
#include <dirent.h>
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <glog/logging.h>

using namespace google::protobuf;


namespace pqfs {

ConfigManager::ConfigManager(std::string config_path):
        config_path(config_path) {
}

// Read the config from a file.
int ConfigManager::Refresh() {
    std::ifstream config_ifs(config_path, std::ios::in | std::ios::binary);
    if (!config_ifs.is_open()) {
        LOG(FATAL) << "ConfigManager: can't open " << config_path;
    }
    google::protobuf::io::IstreamInputStream zero_copy_stream(&config_ifs);
    if (!TextFormat::Merge(&zero_copy_stream, &config)) {
        LOG(ERROR) << "ConfigManager::Refresh: parse failed";
        return EINVAL;
    }
    LOG(INFO) << "ConfigManager::Refresh: " << config.ShortDebugString();
    return 0;
}

} // namespace pqfs
