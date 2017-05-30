/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/*
 * gcc -Wall fuse_lo-plus.c `pkg-config fuse3 --cflags --libs` -o fuse_lo-plus
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse/fuse_lowlevel.h>
#include <fuse/fuse_opt.h>
#include <glog/logging.h>
#include <gperftools/profiler.h>
#include <gperftools/heap-checker.h>
#include "KVStore.h"
#include "RocksDBFactory.h"
#include "LevelDBFactory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config/config_manager.h"
#include "eviction_agent.h"
#include "file_test_util.h"
#include "fs_change_log.h"
#include "fuse_kvfs_ops.h"
#include "kvfs_mount.h"
#include "tiering_in_agent.h"
#include "tiering_out_agent.h"
#include "web_controller.h"

//#include "super.pb.h"   // for ShutdownProtobufLibrary() extern.


struct FkOptions {
    int use_rocksdb;
    char* cloud_proxy_dir;
    char* config_file;
    int force_leak;
} fk_options;

#define FK_OPTION(t, p, v) { t, offsetof(struct FkOptions, p), v }

static const struct fuse_opt fk_opts[] = {
    FUSE_OPT_KEY("--v=%d", FUSE_OPT_KEY_DISCARD),  // for google logging
    FUSE_OPT_KEY("--logtostderr=%d", FUSE_OPT_KEY_DISCARD),  // for google logging
    FUSE_OPT_KEY("--stderrthreshold=%d", FUSE_OPT_KEY_DISCARD),  // for google logging
    FUSE_OPT_KEY("--log_dir=%s", FUSE_OPT_KEY_DISCARD),  // for google logging
    FUSE_OPT_KEY("--minloglevel=%d", FUSE_OPT_KEY_DISCARD),  // for google logging
    FK_OPTION("--use_rocksdb=%d", use_rocksdb, 0),  // for using rocksdb
    FK_OPTION("--force_leak=%d", force_leak, 0),
    FK_OPTION("--config_file=%s", config_file, 0),   // proto, text formatted.
    FK_OPTION("--cloud_proxy_dir=%s", cloud_proxy_dir, 0),
    FUSE_OPT_END
};

void LogArgs(int argc, char *argv[])
{
    std::ostringstream args;
    for (int i = 0; i < argc; i++) {
        args << " '" << argv[i] << "'";
    }
    LOG(INFO) << "Arguments: " << args.str();
}

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    LogArgs(argc, argv);
    // Weird interactionbetween google-logging and fuse args parsing.
    // This log call (before letting fuse mangle the args) helps.
    LOG(INFO) << "VLOG_IS_ON(1) " << VLOG_IS_ON(1); 

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &fk_options, fk_opts, NULL) == -1) {
        LOG(FATAL) << "main: fuse_opt_parse failed.";
    }
    std::ostringstream proxy_base;
    std::string local_proxy_dir;
    std::string cloud_proxy_dir;
    pqfs::CloudProxy* cloud_proxy = nullptr;
    struct fuse_chan *ch;
    char *mountpoint;
    int fuse_loop_ret = 0;
    HeapLeakChecker checker("fuse_kvfs_main.cc");
    // For testing heap checking, force a memory leak.
    if (fk_options.force_leak > 0) {
        auto* leak = malloc(fk_options.force_leak);
        LOG(INFO) << "Forcing leak of " << fk_options.force_leak
            << " bytes at location " << leak;
    }
    // The config file (config.proto format) specifies most of the parameters
    // for the filesystem being mounted.
    LOG_ASSERT(fk_options.config_file != nullptr);
    pqfs::ConfigManager config_manager(fk_options.config_file);
    int error = config_manager.Refresh();
    if (error != 0) {
        LOG(FATAL) << "bad config file '" << fk_options.config_file << "'";
    }
    const auto* config = config_manager.GetConfig();
    /*
     * TODO(joe): now merge any proto specified via --config_proto="....".
     */
    KVStore* store = 0;
    LOG(INFO) << "Fuse_Kfs rocksdb is " << fk_options.use_rocksdb << " shown";
    if (fk_options.use_rocksdb) {
        RocksDBFactory factory;
        store = factory.createLocalHDInstance();
        LOG(INFO) << "Fuse_Kfs RocksDB";
    } else {
        LevelDBFactory factory;
        store = factory.createLocalHDInstance();
        LOG(INFO) << "Fuse_Kfs LevelDB";
    }

    // TODO: store->open(name)?
    FileSystemManager filesystem_manager(store);

    // TODO(joe): would be good to not have fuse determine our command line args,
    // but its api is pretty tied to argc/argv, so would be some work to construct
    // exactly the args it wants.
    int multithreaded = 0;  // out variables from parse, below. ignored.
    int foreground = 0;     // out variables from parse, below. ignored.
    if (fuse_parse_cmdline(&args, &mountpoint, &multithreaded, &foreground) == -1) {
        LOG(FATAL) << "fuse_parse_cmdline failed";
    }
    LOG(INFO) << "VLOG_IS_ON(2) " << VLOG_IS_ON(2); 
    LOG(INFO) << "VLOG_IS_ON(3) " << VLOG_IS_ON(3); 
    proxy_base << "proxy_dir/" << TEST_FSID;
    local_proxy_dir = proxy_base.str();
    // TODO: proxy dir is under current working dir. Arg or env var?
    if (fk_options.cloud_proxy_dir != nullptr) {
        cloud_proxy_dir = fk_options.cloud_proxy_dir;
    }
    if (cloud_proxy_dir.empty()) {
        if (!config->has_cloud() || !config->cloud().has_endpoint()) {
            LOG(FATAL) << "no cloud_proxy_dir specified";
        }
        cloud_proxy_dir = config->cloud().endpoint();
    }

    LOG(INFO) << "proxy " << local_proxy_dir << " <==> " << cloud_proxy_dir;
    // TODO: why not local?
    cloud_proxy = new pqfs::CloudProxy(local_proxy_dir, cloud_proxy_dir);

    KvfsMount kvfs_mount(store, "mykvfs.db", TEST_FSID, cloud_proxy);

    // Read in current Index, if any.
    pqfs::IndexManager index_manager(cloud_proxy, pqfs::IndexManager::TEXT_FORMAT);
    index_manager.Refresh();

    // filesystem_manager.setTieringInAgent(&tiering_in_agent);

    // TODO(): args or config.
    kvfs_mount.set_fuse_keep_cache(config->primary().fuse_cache_enable());
    // TODO: remove this - trying direct io to see if it fixes a
    // non-deterministic problem in fsx testing.
    kvfs_mount.set_fuse_direct_io(config->primary().fuse_direct_io());

    LOG(INFO) << "keep_cache=" << kvfs_mount.get_fuse_keep_cache()
        << " attr_timeout " << kvfs_mount.get_fuse_attr_timeout()
        << " entry_timeout " << kvfs_mount.get_fuse_entry_timeout();
    kvfs_mount.Init(&filesystem_manager);
    // TODO: Create change long here (to be provided to mount, tiering, eviction).
    // ### NEEDSWORK:
    //std::string directory_template("/tmp/seg_XXXXXX");
    pqfs::SegmentFactory segment_factory(
            std::string("./seg_"),  // working directory template.
            cloud_proxy,
            kvfs_mount.get_fs_manager(),
            TEST_FSID);  // TODO(): why needed?

    pqfs::TieringInAgent tiering_in_agent(
            &index_manager,      // not owned.
            cloud_proxy,          // not owned.
            &segment_factory); // not owned.
    kvfs_mount.set_tiering_in_agent(&tiering_in_agent);

    // Create tiering out agent
    pqfs::TieringOutAgent tiering_out_agent(
        &kvfs_mount,
        kvfs_mount.get_change_log(),
        cloud_proxy,
        &index_manager,
        &segment_factory);

    tiering_out_agent.Start();

    pqfs::EvictionAgent eviction_agent(&kvfs_mount, kvfs_mount.get_change_log());

    eviction_agent.Start();

    // Create eviction agent

    // Create restful web-controller 
    // TODO(joe): local config file
    const char *port = getenv("WEB_SERVER_PORT");
    if (port == nullptr) {
	LOG(FATAL) << "WEB_SERVER_PORT not specified.";
    }
    int port_num = atoi(port);
    if (port_num <= 1024) {
        LOG(FATAL) << "Invalid WEB_SERVER_PORT "
		<< port << "(" << port_num << ")";
    }
    pqfs::WebController web_controller(
            &tiering_out_agent, &eviction_agent, port_num);
    web_controller.Start();

    // TODO(): should only be done by specific request.
    pqfs::CreateRoot(kvfs_mount.get_fs_manager(), kvfs_mount.get_fs_id());
    // This was starting tiering.
    kvfs_mount.Start();
    if ((ch = fuse_mount(mountpoint, &args)) == nullptr) {
        LOG(ERROR) << "fuse_mount failed";
        goto out;
    }
    if (!VLOG_IS_ON(1)) {
        LOG(WARNING) << "VLOG_IS_ON(1) false!";
    }
    LOG(INFO) << "starting fuse";
    VLOG(1) << "starting fuse";
    struct fuse_session *se;
    se = fuse_lowlevel_new(&args, fk_init_oper(), fk_oper_size(), &kvfs_mount);
    if (se != NULL) {
        if (fuse_set_signal_handlers(se) != -1) {
            fuse_session_add_chan(se, ch);
            // session loop returns when the unmount is attempted on filesystem.
            fuse_loop_ret = fuse_session_loop(se);
            LOG(INFO) << "main: fuse_loop_ret " << fuse_loop_ret;
            fuse_remove_signal_handlers(se);
            fuse_session_remove_chan(ch);
        }
        fuse_session_destroy(se);
    }
    eviction_agent.RequestStop();
    tiering_out_agent.RequestStop();
    web_controller.RequestStop();
    eviction_agent.Join();
    tiering_out_agent.Join();
    web_controller.Join();

    fuse_unmount(mountpoint, ch);
    free(mountpoint);

out:
    kvfs_mount.Shutdown();
    delete cloud_proxy;
    store->close();
    delete store;
    sleep(1);   // TODO: Need way to shutdown leveldb threads:
                // https://github.com/basho/leveldb/wiki/mv-valgrind-cleanup
                // TODO: Need way to shutdown rocksdb threads:
                // https://github.com/basho/rocksdb/wiki/mv-valgrind-cleanup

    fuse_opt_free_args(&args);
    google::protobuf::ShutdownProtobufLibrary();
    // This shouldn't be necessary, but for some reason just defining CPUPROFILE
    // environment var isn't causing the flush. TODO: fix/understand.
    if (ProfilingIsEnabledForAllThreads())
        ProfilerFlush();
    google::ShutdownGoogleLogging();
    EXPECT(checker.NoLeaks());
}
