#!/bin/sh
#
# odd_proxy_example.sh [proxy_host [proxy_port]]
#
# Requirements:
#     - assumes you have ncat installed
#

if [ -d ../exp ] ; then
    chdir ..
fi

if [ ! -d ./exp/perl/blib ] ; then
    echo "$0: Setting up the Perl prototype environment..."
    (cd ./exp/perl && perl Build.PL && ./Build)
fi

if [ ! -x ./exp/perl/bin/connect ] ; then
    echo "$0: I need App::TTT unpacked into ./exp/perl and then run from package root."
    exit 1
fi

PATH=$PATH:./bin:./ext/perl/bin
PERLLIB=$PERLLIB:./ext/perl/lib

echo "Point your browser to localhost:3128. Press Ctrl-C to quit proxy."
ncat -l -k localhost 3128 -c "perl -I./exp/perl/lib ./exp/perl/bin/connect | ./src/log | sed -El -e 's/ ([a-z]+) ([a-z]+) / \2 \1 /g'"

