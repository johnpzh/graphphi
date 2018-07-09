#! /usr/bin/bash
app=sssp

#echo "+------------------------------------------------+"
# Pokec
./$app /data/zpeng/pokec/soc-pokec 8192 128 -w

# Twitter
./$app /data/zpeng/twt/out.twitter 65536 8 -w

#echo "|"
#echo "+------------------------------------------------+"
