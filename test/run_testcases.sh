#! /bin/bash
(( PASSED = 0 ))
(( FAILED = 0 ))

for T in $ORIG_DIR/testcases/*
do
# Tests assume running in $TEST_DIR, with mount point "mount"
    if $T
    then
        (( PASSED++ ))
        PASS_LIST="$PASS_LIST $T"
    else
        (( FAILED++ ))
        FAIL_LIST="$FAIL_LIST $T"
    fi
# Tests should clean up after themselves,
    echo "After $T: $( ls -l mount)"
# but try to avoid leftover junk from one failed test from breaking the next.
    rm -f mount/*
done

echo "Done with tests."
echo "$PASSED PASSED:"
ls -l $PASS_LIST 
echo "$FAILED FAILED:"
if [ -n "$FAIL_LIST" ]
then
    ls -l $FAIL_LIST
    exit 1
fi
exit 0
