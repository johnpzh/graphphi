#! /usr/bin/bash

DIRS=*/

for dir in ${DIRS}; do
	(cd $dir && \
		sed -i "s/##//" run.sh)
done

