#! /bin/bash
# Script which generates a config file based on a template
# in a bash here-document.
# Unfortunately, this file is both script and data. Ugh.
# (don't know how to make the full here doc substitutions in external file).
#
# Usage: $0 <output_config.proto>

cat > $1 <<HEREDOC
# This file was generated by ${0}
# It contains an instance of pqfs::config::proto::Config in text format.
# (which is parsed by our filesystem server, fuse_kvfs).
# The text protocol buffer format should be pretty obvious (json-like).
# https://developers.google.com/protocol-buffers/docs/overview
# Because we're in a here-document, shell variable substitions apply.

name: "${CONFIG_NAME-test_pqfs_filesystem}"
#    optional string uuid = 2;
# Real uuid here?
uuid: "${TEST_FS_BASE}"

# optional Primary primary = 3;
primary {
    host: "localhost"
    # not used yet.
    epoch: 1
    # millis since epoch (date +%s000)
    #1484799532000
    startTime: $(date +%s000)

    # For testing, we disable caching in fuse.
    fuse_cache_enable: ${FUSE_CACHE_ENABLE-true}
    # I wouldn't if I were you (not sure what direct does).
    fuse_direct_io: ${FUSE_DIRECT_IO-false}
}

keyValueStore {
    type: LEVELDB
    # name?
    filename: "${LEVELDB_KVFS_STORE_NAME-mykvfs.db}"
    persistenceRule: LAZY  # todo
    maxSizeBytes: 100000000  # todo
}

tiering {
    cacheDir: "${PROXY_DIR-proxy_dir}"

    # TODO: rest NYI.
    encryption: UNENCRYPTED

    encryptionKey: "/path/to/key"

    compression: ZIP

    mirrorLagSeconds: 0  # ??

    cacheSizeBytes: 100000000

    cacheFreePercent: 20

    maxObjectSizeBytes: 1000000000
}

cloud {
    protocol: ${CLOUD_PROTOCOL-AMAZON_S3}
    # optional string endpoint = 2;
    endpoint: "s3:CBFS/${TEST_FS_BASE}/proxy"
}

HEREDOC
