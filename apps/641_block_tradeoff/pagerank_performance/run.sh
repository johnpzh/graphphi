#!/usr/bin/bash

data_file="/data/zpeng/twt/out.twitter"
echo "+------------------------------------------------+"
# Stripe Length
max=0
index=0
for ((i = 8; i <= 8192; i *= 2)); do
#	numactl -m 0 ./pagerank ${data_file} 4096 ${i}
	./pagerank /data/zpeng/pokec/soc-pokec 8192 128
	source mean.txt
	v[index]=$MEAN
	((index++))
	if (( $(echo "$MEAN > $max" | bc -l) )); then
		max=$MEAN
	fi
done
echo "| Normalized Performance:"
for ((i = 0; i < index; ++i)); do
	echo "| $(echo "${v[i]} / $max" | bc -l)"
done
echo "+------------------------------------------------+"