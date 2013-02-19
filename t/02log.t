#!/usr/bin/env perl

use warnings;
use strict;

use lib './t/lib';

use Test::Command tests => 9;
use Test::More;

BEGIN {
    $ENV{PATH} = './src:.:' . $ENV{PATH};
}


my @TEST_FILES = ( './t/test-data/sample-request-get.txt', './t/test-data/sample-response-302.txt' );

# 1-6: simple requests 
foreach my $testf ( @TEST_FILES ) {
    my $cmd = Test::Command->new( cmd => "log < '$testf'" );
    $cmd->stdout_is_file($testf, "log does not molest single http message from $testf");
    $cmd->stderr_like(qr/^log.c: \[(\w\w\w)\]/, "log message was generated from $testf");
    $cmd->exit_is_num(0, "log exited with zero during $testf");
}

# 7-8: request + response
my $expected = `cat @TEST_FILES`;
my $cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not molest concatentated http messages');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages');

# 9: num log messages == num http messages
my @stderr_lines = $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated');

# 10-12: double the test data
#push @TEST_FILES, @TEST_FILES;
#$expected = `cat @TEST_FILES`;
#$cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
#$cmd->stdout_is_eq($expected, 'log does not molest concatenated http messages (repeated)');
#$cmd->exit_is_num(0, 'log exited with zero for (req + res) x 2');
#@stderr_lines = $cmd->stderr_value;
#is( $#TEST_FILES, $#stderr_lines, '1 log message for each http message generated');
