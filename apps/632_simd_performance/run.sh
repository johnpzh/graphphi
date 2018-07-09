#! /usr/bin/bash

subject="\$6.3.2 SIMD Speedup"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./run.sh)
done

echo "==================== End ${subject} ===================="
