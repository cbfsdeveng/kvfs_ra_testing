
In this directory:

build.sh builds fuse_kvfs
It checks the following environment variables:

NO_CLEAN - to suppress a make-clean before each build, set NO_CLEAN=false

BUILD_MODE - controls is either "release" (optimized) or "debug" (default).

RUN_FAST - enables optimized build and running with debugs turned production/release
levels.

e.g. NO_CLEAN=false BUILD_MODE=debug ./build.sh

for performance testing runs:
e.g. NO_CLEAN=false RUN_FAST=true ./build.sh

test_shell.sh mounts an empty filesystem and runs a shell in it's private
directory.

The directory "mount" under that shell's cwd is the mount point.

With no other arguments, test_shell.sh run a shell in the private directory so
you can run tests here manually against mount/. When you exit that shell, the
filesystem will be unmounted.

You can also use test_shell.sh to run a command (in which case, the script
unmounts and exits when the run is complete). You must specify a full pathname
for the command.

test_shell.sh invokes build.sh, so it the previously mentioned build environment
variables work the same.

test_shell.sh also has its own set of environment variables to control things
like verbosity and profiling. See its source code for details.

e.g.  time BUILD_MODE=release CPUPROFILE="" FUSE_DEBUG="" GLOG_v=0 ./test_shell.sh $PWD/qa/fio/runfio 2>&1


test_basic.sh uses test_shell.sh to run a some basic kick-the-tires tests.

PROFILING:

by default, test_shell enables cpu profiling on fuse_kvfs for the mount. On exit
(unmount), profiling data will be written to /tmp/fuse_kvfs.prof (default).
You can then analyze the profile data via google-pprof.  e.g.:

  google-pprof ../src/fuse_kvfs /tmp/fuse_kvfs.prof --web

Will produce an output file named something like /tmp/pprof9311.1.svg
You can view this in a web browser (probably by scp'ing it back to your machine).
google-pprof also has text-based reports. "man google-pprof" for more info.

