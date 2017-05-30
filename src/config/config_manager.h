/*
 * API used to access the change log.
 * Used by both writers and consumers.
 * 
 * Change-log may be written to KVS?
 * Else in files (data format?).
 */
#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include "config.pb.h"

namespace pqfs {


class ConfigManager {
public:
    // ConfigManager();  // defaults.
    ConfigManager(std::string config_path);

    // Read the latest log from persistent storage.
    int Refresh();

    // get current config.
    const pqfs::config::proto::Config* GetConfig() {
        return &config;
    }

private:
    // log_dir is the home of our persistent logs.
    std::string config_path;
    pqfs::config::proto::Config config;
};

} // namespace pqfs
#endif // CONFIG_MANAGER_H_
