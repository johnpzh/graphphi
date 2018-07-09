#! /usr/bin/bash

subject="\$6.4.3 SIMD Utilization"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./run.sh)
done

echo "==================== End ${subject} ===================="
