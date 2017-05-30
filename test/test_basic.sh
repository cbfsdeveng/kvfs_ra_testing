#! /bin/bash
# Run all scripts (testcases) under the testcases directory located in
# same directory as this scripti ($0)

TEST_DIR=$(echo "$0" | sed 's,[^/]*$,,')

echo "TEST_DIR $TEST_DIR"
if ! echo "$TEST_DIR" | grep '^/'
then
TEST_DIR=$PWD
fi

./test_shell.sh $TEST_DIR/run_testcases.sh
