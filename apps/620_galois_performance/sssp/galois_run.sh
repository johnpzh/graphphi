#! /usr/bin/bash

if [[ $# -lt 1 ]]; then
	count=10
else
	count=$1
fi

app=sssp/sssp
galois_dir="/home/zpeng/code/galois/build/release/apps"

function execute {
	# execute (data)
	echo "**************************************************"
	echo "* Benchmark: Galois $app"
	echo "* Data: $1"
	output=output.txt
	:> $output
	for ((i = 0; i < count; ++i)); do
		${galois_dir}/$app $1 -startNode=0 -t=64 -algo=asyncPP -noverify >> $output
	done
	python ../../../tools/galois/get_galois_output.py $output
	echo "* Galois $app finished."
	echo "**************************************************"
}

# Pokec
data=/data/zpeng/pokec/soc-pokec.gr
execute $data

# LiveJournal
data=/data/zpeng/livejournal/livejournal.gr 
execute $data

# RMAT24
data=/data/zpeng/rmat24/rmat24.gr
execute $data

# RMAT27
data=/data/zpeng/rmat27/rmat27.gr
execute $data

# Twitter
data=/data/zpeng/twt/out.twitter.gr
execute $data

# Friendster
data=/data/zpeng/friendster/friendster.gr
execute $data
