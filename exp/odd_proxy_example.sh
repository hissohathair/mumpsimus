#!/bin/sh
#
# odd_proxy_example.sh [http_proxy_host [http_proxy_port]]
#
# Disables the cache and gzip encoding by modifying request headers,
# then mucks up the message body with a simple sed trick.
#
# Status: Kind of works for simple web pages but the pipeline gets
# confused and into an uncrecoverable state if mixed media is piped
# through it.
#
# Requirements:
#     - assumes you have ncat installed
#     - http_proxy_host is hostname of proxy (port is port num)
#

if [ "x$1" = "x" ] ; then
    http_proxy_host=proxy
else
    http_proxy_host=$1
fi

if [ "x$2" = "x" ] ; then
    http_proxy_port=3128
else
    http_proxy_port=$2
fi

# Temporarily modify path if utils not installed yet
PATH=$PATH:./src:../src:.

echo "Point your browser to localhost:3128. Press Ctrl-C to quit proxy."
ncat -l -k localhost 3128 -c "headers -c \"sed -El -e 's/^Cache-Control: .*/Cache-Control: no-cache/g'  -e 's/If-Modified-Since: .*/X-If-Modified-Since: none/g' -e 's/Accept-Encoding: .*/X-Accept-Encoding: none/g'\" | log -v | ncat $http_proxy_host $http_proxy_port | body -c \"sed -El -e 's/ ([a-z]+) ([a-z]+) / \2 \1 /g'\" | log -v"

