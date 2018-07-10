#! /usr/bin/bash
app=mis

# Pokec
args="/data/zpeng/pokec/soc-pokec 8192 128"
echo "+------------------------------------------------+"
# DRAM
echo "| DRAM:"
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
echo "| MCDRAM:"
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"

#######################################################
# Twitter
args="/data/zpeng/twt/out.twitter 16384 1024"
echo "+------------------------------------------------+"
# DRAM
echo "| DRAM:"
./${app} ${args}
source mean.txt
dram_mean=$MEAN

# MCDRAM
echo "| MCDRAM:"
numactl -p 1 ./${app} ${args}
source mean.txt
hbm_mean=$MEAN

speedup=$(echo "$dram_mean / $hbm_mean" | bc -l)

echo "| HBM Speedup over DRAM: ${speedup}"
echo "+------------------------------------------------+"
