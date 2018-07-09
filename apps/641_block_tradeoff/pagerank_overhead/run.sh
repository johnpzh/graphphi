#!/usr/bin/bash
make clean 
make debug=1
#make

# Stripe Length
data_file="/data/zpeng/twt/out.twitter"
for ((sl = 8; sl <= 8192; sl *= 2)); do
	report_dir="report_$(date +%Y%m%d-%H%M%S)_stripe-length-${sl}"
	#amplxe-cl -collect hpc-performance -result-dir ${report_dir} -data-limit=0 -- numactl -m 0 ./pagerank ${data_file} 4096 ${sl}
	amplxe-cl -collect concurrency -knob enable-user-tasks=true -knob enable-user-sync=true -knob analyze-openmp=true -result-dir ${report_dir} -data-limit=0 -- numactl -m 0 ./pagerank ${data_file} 4096 ${sl}
	amplxe-cl -report summary -result-dir ${report_dir} -format text -report-output ${report_dir}/report.txt
done
