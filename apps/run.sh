#! /usr/bin/bash

subject="GraphPhi Evaluation"
echo "-------------------- ${subject} --------------------"
DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && ./run.sh)
done

echo "-------------------- End ${subject} --------------------"
