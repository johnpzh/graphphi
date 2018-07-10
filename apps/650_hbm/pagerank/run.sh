#! /usr/bin/bash
app=pagerank

# Pokec
args="/data/zpeng/pokec/soc-pokec 4096 16"
echo "+------------------------------------------------+"
# DRAM
echo "| DRAM:"
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
echo "| HBM:"
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"

#######################################################
# Twitter
args="/data/zpeng/twt/out.twitter 32768 1024"
echo "+------------------------------------------------+"
# DRAM
echo "| DRAM:"
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
echo "| HBM:"
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"