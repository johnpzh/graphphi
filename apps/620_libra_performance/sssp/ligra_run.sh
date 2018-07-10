#! /usr/bin/bash

if [[ $# -lt 1  ]]; then
	rounds=10
else
	rounds=$1
fi

app=BellmanFord
apps_dir="/home/zpeng/code/ligra/apps"

function execute {
	# execute (data)
	echo "**************************************************"
	echo "* Benchmark: Ligra $app"
	echo "* Data: $1"
	output=output.txt
	:> $output
	${apps_dir}/$app $1 -rounds ${rounds} ${1}_weighted.adj >> $output
	python ../../../tools/ligra/get_ligra_output.py $output
	echo "* Ligra $app finished."
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
