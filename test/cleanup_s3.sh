#! /bin/bash -x

# Remove the files created by test_shell.sh
s3cmd ls "s3://CBFS/kvfs_work.*" |sed 's/.* //' > /tmp/del_list.$$
xargs --verbose < /tmp/del_list.$$  s3cmd del -v --recursive
rm /tmp/del_list.$$

# Artifact from running fuse_kvfs with default params.
s3cmd del -v --recursive s3://CBFS/proxy_dir

rm -rfv /tmp/seg_??????/
rm -rfv /tmp/kvfs_work.???*/

rm -f /tmp/fuse_kvfs.*.heap

# Big hammer
sudo umount -a -t fuse.fuse_kvfs
