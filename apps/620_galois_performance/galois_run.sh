#! /usr/bin/bash

if [[ $# -lt 1  ]]; then
	count=10
else
	count=$1
fi

subject="\$6.2 Galois Overall Performance"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./galois_run.sh $count)
done

echo "==================== End ${subject} ===================="
