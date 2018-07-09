#! /usr/bin/bash
app=sssp

# Pokec
args="/data/zpeng/pokec/soc-pokec 8192 128 -w"
echo "+------------------------------------------------+"
# DRAM
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"

#######################################################
# Twitter
args="/data/zpeng/twt/out.twitter 65536 8 -w"
echo "+------------------------------------------------+"
# DRAM
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"
