#! /bin/bash -x
# Verify that heap checking is enabled and working.
Run basic test, with and without a forced leak. Should succeed, then fail.
./test_shell.sh true
NO_LEAK_ERRNO=$?
if [ NO_LEAK_ERRNO -ne 0 ]
then
echo "test_shell.sh failed (${NO_LEAK_ERRNO}). Not testing forced leak."
exit 1
fi

FUSE_OPTS=--force_leak=9999 ./test_shell.sh true
LEAK_ERRNO=$?
if [ "$LEAK_ERRNO" -eq 0 ]
then
echo "Heap checking failed to catch forced leak! ERROR!!!"
else
echo "Heap checking working as expected."
fi
