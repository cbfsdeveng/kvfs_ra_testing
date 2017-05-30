/*
 * Run some tests on the tiering agent.
 */
#include "expect.h"
#include <glog/logging.h>
#ifdef PROTOBUF_V3  // nice to have, hassle to switch?
#include <google/protobuf/util/message_differencer.h>
#endif
#include "config.pb.h"
// NEEDSWORK: IWYU
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <stdio.h>
#include <fstream>
#include <string>

using namespace google::protobuf;
using namespace google::protobuf::io;

int ReadConfig(
        pqfs::config::proto::Config* config,
        std::string& pathname) {
    int ret = 0;
    int fd = open(pathname.c_str(), 0);
    if (fd < 0) {
        ret = errno ? errno : EIO;
        LOG(ERROR) << "ReadConfig " << pathname << " error " << ret;
    } else {
        config->Clear();
        FileInputStream istream(fd);
        if (!TextFormat::Merge(&istream, config)) {
            ret = errno ? errno : EIO;
        } else {
            LOG(INFO) << "ReadConfig: " << config->DebugString();
        }
    }
    return ret;
}

int WriteConfig(
        pqfs::config::proto::Config& config,
        std::string& pathname) {
    int ret = 0;
    LOG(INFO) << "WriteConfig " << config.DebugString()
        << " to " << pathname;
    int fd = creat(pathname.c_str(), 0777);
    if (fd < 0) {
        ret = errno ? errno : EIO;
    } else {
        FileOutputStream ostream(fd);
        if (!TextFormat::Print(config, &ostream)) {
            ret = errno ? errno : EIO;
        }
    }
    if (ret) {
        LOG(ERROR) << "WriteConfig err " << ret << " "  << pathname;
    }
    close(fd);  // harmless to close -1
    return ret;
}


// TODO: use googletest and googlemock.
int
main(int argc, char **argv) {
    pqfs::config::proto::Config config;
    std::string sample_file = "sampleconfig.txt";
    std::string out_file = "/tmp/config.txt";
    ReadConfig(&config, sample_file);
    WriteConfig(config, out_file);
}
