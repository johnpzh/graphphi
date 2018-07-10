#! /usr/bin/bash

if [[ $# -lt 1 ]]; then
	count=10
else
	count=$1
fi

app=betweennesscentrality/betweennesscentrality-inner
galois_dir="/home/zpeng/code/galois/build/release/apps"

function execute {
	# execute (data)
	echo "**************************************************"
	echo "* Benchmark: Galois $app"
	echo "* Data: $1"
	output=output.txt
	:> $output
	for ((i = 0; i < count; ++i)); do
		${galois_dir}/$app ${1}.vgr -algo=async -graphTranspose=${1}.tvgr -noverify -startNode=0 -t=1  >> $output
	done
	python ../../../tools/galois/get_galois_output.py $output
	echo "* Galois $app finished."
	echo "**************************************************"
}

# Pokec
data=/data/zpeng/pokec/soc-pokec
execute $data

# LiveJournal
data=/data/zpeng/livejournal/livejournal 
execute $data

# RMAT24
data=/data/zpeng/rmat24/rmat24
execute $data

# RMAT27
data=/data/zpeng/rmat27/rmat27
execute $data

# Twitter
data=/data/zpeng/twt/out.twitter
execute $data

# Friendster
data=/data/zpeng/friendster/friendster
execute $data
