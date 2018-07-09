#! /usr/bin/bash
app=pagerank

#echo "+------------------------------------------------+"
# Pokec
./$app /data/zpeng/pokec/soc-pokec 4096 16

# Livejournal
./$app /data/zpeng/livejournal/livejournal 16384 256

# RMAT24
./$app /data/zpeng/rmat24/rmat24 32768 512

# RMAT27
./$app /data/zpeng/rmat27/rmat27 16384 8192

# Twitter
./$app /data/zpeng/twt/out.twitter 32768 1024

# Friendster
./$app /data/zpeng/friendster/friendster 131072 512

#echo "|"
#echo "+------------------------------------------------+"
