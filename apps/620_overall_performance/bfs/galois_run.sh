#! /usr/bin/bash

if [[ $# -lt 1 ]]; then
	count=10
else
	count=$1
fi

app=bfs/bfs
galois_dir="/home/zpeng/code/galois/build/release/apps"
data_dir=/data/zpeng
# Pokec
echo "**************************************************"
echo "* Benchmark: Galois $app"
echo "* Data: ${data_dir}/pokec/soc-pokec.vgr"
output=output.txt
:> $output
for ((i = 0; i < count; ++i)); do
	${galois_dir}/${app} ${data_dir}/pokec/soc-pokec.vgr -startNode=0 -t=${power} -algo=barrierWithCas -noverify >> $output
done
python ../../../tools/galois/get_galois_output.py $output
echo "* Galois $app finished."
echo "**************************************************"

# LiveJournal
echo "**************************************************"
echo "* Benchmark: Galois $app"
echo "* Data: ${data_dir}/livejournal/livejournal.vgr"
output=output.txt
:> $output
for ((i = 0; i < count; ++i)); do
	${galois_dir}/${app} ${data_dir}/livejournal/livejournal.vgr -startNode=0 -t=${power} -algo=barrierWithCas -noverify >> $output
done
python ../../../tools/galois/get_galois_output.py $output
echo "* Galois $app finished."
echo "**************************************************"

# RMAT24

# RMAT27

# Twitter

# Friendster
