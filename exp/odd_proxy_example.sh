#!/bin/sh

if [ -d ../exp ] ; then
    chdir ..
fi

if [ ! -x ./exp/perl/bin/connect ] ; then
    echo "$0: I need App::TTT unpacked into ./exp/perl and then run from package root."
    exit 1
fi

PATH=$PATH:./bin:./ext/perl/bin
PERLLIB=$PERLLIB:./ext/perl/lib

ncat -l -k localhost 3128 -c "perl -I./exp/perl/lib ./exp/perl/bin/connect | sed -El -e 's/ ([a-z]+) ([a-z]+) / \2 \1 /g'"

