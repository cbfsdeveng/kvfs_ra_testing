#! /bin/bash

((FAILED=0))
((SUCCEEDED=0))

set -x
if $ORIG_DIR/qa/fsx/fsx -N 1000 mount/fsxfile > fsx.out 2>&1
then
((++SUCCEEDED))
rm mount/fsxfile mount/fsxfile.fsxgood  mount/fsxfile.fsxlog
else
echo "$0: fsx failed"
((++FAILED))
mv mount/fsxfile mount/fsxfile.fsxgood  mount/fsxfile.fsxlog $TEST_DIR
fi

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
