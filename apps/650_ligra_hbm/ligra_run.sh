#! /usr/bin/bash

if [[ $# -lt 1  ]]; then
	rounds=10
else
	rounds=$1
fi

subject="\$6.2 Ligra Overall Performance"
echo "==================== ${subject} ===================="
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./ligra_run.sh $rounds)
done

echo "==================== End ${subject} ===================="
