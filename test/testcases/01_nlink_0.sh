#! /bin/bash

# Various tests about number of links on files and dirs.

((FAILED=0))
((SUCCEEDED=0))

if mkdir mount/nlink_dir
then
((++SUCCEEDED))
else
echo "$0: mkdir failed?"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/nlink_dir)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 2 ]
then
echo "nlink_dir: bad link count (should be 2): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

if mkdir mount/nlink_dir/nlink_subdir
then
((++SUCCEEDED))
else
echo "$0: mkdir failed?"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/nlink_dir)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 3 ]
then
echo "nlink_dir: bad link count after subdir (should be 3): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

read -a OUT <<< "$(ls -ld mount/nlink_dir/nlink_subdir)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 2 ]
then
echo "nlink_dir/nlink_subdir: bad link count (should be 2): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

echo "Hello link!" > mount/nlink_dir/nlink_hello
ln mount/nlink_dir/nlink_hello mount/nlink_dir/nlink_hello_link
read -a OUT <<< "$(ls -ld mount/nlink_dir/nlink_hello_link)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 2 ]
then
echo "nlink_hello: bad link count (should be 2): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

if rm mount/nlink_dir/nlink_hello
then
((++SUCCEEDED))
else
echo "failed to remove nlink_hello"
((++FAILED))
fi

read -a OUT <<< "$(ls -ld mount/nlink_dir/nlink_hello_link)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 1 ]
then
echo "nlink_hello: bad link count (should be 1): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

read -a OUT <<< "$(ls -ld mount/nlink_dir)"
echo "ls output: ${OUT[*]}"
if [ ${OUT[1]} -ne 3 ]
then
echo "nlink_hello: bad dir link count (should be 3): ${OUT[1]}"
((++FAILED))
else
((++SUCCEEDED))
fi

if rm mount/nlink_dir/nlink_hello_link
then
((++SUCCEEDED))
else
echo "failed to remove nlink_hello_link"
((++FAILED))
fi


if rmdir mount/nlink_dir/nlink_subdir
then
((++SUCCEEDED))
else
echo "failed to remove nlink_subdir"
((++FAILED))
fi

if rmdir mount/nlink_dir
then
((++SUCCEEDED))
else
echo "failed to remove nlink_dir"
((++FAILED))
fi

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
