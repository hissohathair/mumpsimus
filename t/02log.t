#!/usr/bin/env perl

use warnings;
use strict;

use Test::Command tests => 7;

BEGIN {
    $ENV{PATH} = './src:.:' . $ENV{PATH};
}


my @TEST_FILES = ( './t/test-data/sample-request-get.txt', './t/test-data/sample-response-302.txt' );

# 1-6: simple requests 
foreach my $testf ( @TEST_FILES ) {
    my $cmd = Test::Command->new( cmd => "log < '$testf'" );
    $cmd->stdout_is_file($testf, 'log does not molest single http messages');
    $cmd->stderr_like(qr/^log.c: \[(\w\w\w)\]/);
    $cmd->exit_is_num(0, "log exited with zero during $testf");
}

# 7: request + response
my $expected = `cat @TEST_FILES`;
my $cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not molest req + res');
#$cmd->exit_is_num(0, 'log exited with zero');

