#! /usr/bin/bash

if [[ $# -lt 1 ]]; then
	count=10
else
	count=$1
fi

app=independentset/independentset
galois_dir="/home/zpeng/code/galois/build/release/apps"

function execute {
	# execute (data)
	echo "**************************************************"
	echo "* Benchmark: Galois $app"
	echo "* Data: $1"
	output=output.txt
	:> $output
	for ((i = 0; i < count; ++i)); do
		${galois_dir}/$app $1 -startNode=0 -t=64 -nondet -noverify >> $output
	done
	python ../../../tools/galois/get_galois_output.py $output
	echo "* Galois $app finished."
	echo "**************************************************"
}

# Pokec
data=/data/zpeng/pokec/soc-pokec.vgr
execute $data

# LiveJournal
data=/data/zpeng/livejournal/livejournal.vgr 
execute $data

# RMAT24
data=/data/zpeng/rmat24/rmat24.vgr
execute $data

# RMAT27
data=/data/zpeng/rmat27/rmat27.vgr
execute $data

# Twitter
data=/data/zpeng/twt/out.twitter.vgr
execute $data

# Friendster
data=/data/zpeng/friendster/friendster.vgr
execute $data
