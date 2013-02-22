#!/bin/sh
#
# log_proxy_example.sh
#
# Requirements:
#     - assumes you have ncat installed
#     - assumes that HTTP_PROXY and HTTP_PROXY_PORT point at a real proxy server
#

if [ "x$1" = "x" ] ; then
    http_proxy=10.8.0.1
else
    http_proxy=$1
fi
if [ "x$2" = "x" ] ; then
    http_port=3128
else
    http_port=$2
fi

if [ ! -x ./src/log ] ; then
    echo "$0: Run this from package root, and build with make"
    exit 1
fi

echo "Press Ctrl-C to quit proxy. Connecting to $http_proxy:$http_port"
ncat -l -k localhost 3128 -c "./src/log | ncat $http_proxy $http_port | ./src/log -v"

