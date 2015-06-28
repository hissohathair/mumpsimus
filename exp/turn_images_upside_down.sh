#!/bin/sh
#
# turn_images_upside_down.sh [http_proxy_host [http_proxy_port]]
#
# Pipes message bodies through ImageMagick's mogrify to flip an
# image upside down.
#
# Status: Kind of works for single image requests but doesn't
# work well as part of a proxy that's handling non-image requests
# as well.
#
# Requirements:
#     - assumes you have ncat installed
#     - assumes Image Magick is installed
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
ncat -l -k localhost 3128 -c "headers -c \"sed -El -e 's/^Cache-Control: .*/Cache-Control: no-cache/g'  -e 's/If-Modified-Since: .*/X-If-Modified-Since: none/g'\" | log | ncat $http_proxy_host $http_proxy_port | ULOG_LEVEL=6 body -t 'image/gif*' -c \"convert -flip -flop fd:0 fd:1\" | log -v"





