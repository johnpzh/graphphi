#! /usr/bin/bash

subject="\$6.2 Overall Performance"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./run.sh)
done

echo "==================== End ${subject} ===================="
