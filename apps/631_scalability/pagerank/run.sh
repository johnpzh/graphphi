#! /usr/bin/bash
app=pagerank

#echo "+------------------------------------------------+"
# Pokec
./$app /data/zpeng/pokec/soc-pokec 4096 16

# Twitter
./$app /data/zpeng/twt/out.twitter 32768 1024

#echo "|"
#echo "+------------------------------------------------+"
