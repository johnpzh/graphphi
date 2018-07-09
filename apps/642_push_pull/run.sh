#! /usr/bin/bash

subject="\$6.4.2 Push-based Execution"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./run.sh)
done

echo "==================== End ${subject} ===================="
