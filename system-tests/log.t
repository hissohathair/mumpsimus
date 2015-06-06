#!/usr/bin/env perl
#
# log.t:
#
#   System tests for the log command.
#

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 19;
use Test::More;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
    $ENV{TERM} = '';
}

my $COMMAND = 'log';

my @SEARCH_DIR = ( 'test-data', 'system-tests/test-data' );

my $HEAD_TEST_FILE = qx{ find @SEARCH_DIR -name sample-request-get.txt 2>/dev/null };   chomp($HEAD_TEST_FILE);
my $BODY_TEST_FILE = qx{ find @SEARCH_DIR -name sample-response-302.txt 2>/dev/null };  chomp($BODY_TEST_FILE);
my @TEST_FILES = ( $HEAD_TEST_FILE, $BODY_TEST_FILE );


# 1-8: simple requests 
foreach my $testf ( @TEST_FILES ) {
    my $cmd = Test::Command->new( cmd => qq{ $COMMAND < "$testf" } );
    $cmd->stdout_is_file($testf, "log does not corrupt single http message from $testf");
    $cmd->stderr_like(qr/log.c: \[(\w\w\w)\]/, "log message was generated from $testf");
    $cmd->stderr_like(qr/HTTP\/[\d]\.[\d]/, "log message looked vaguely OK");
    $cmd->exit_is_num(0, "log exited with zero during $testf");
}

# 9-10: request + response
my $expected = `cat @TEST_FILES`;
my $cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not corrupt concatentated http messages');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages');

# 11: num log messages == num http messages
my @stderr_lines = split /\n/, $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated');

# 12-15: double the test data
push @TEST_FILES, @TEST_FILES;
is( $#TEST_FILES+1, 4, 'Sanity check: there are 4 test files' );
$expected = `cat @TEST_FILES`;
$cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not corrupt concatenated http messages (doubled)');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages (doubled)');
@stderr_lines = split /\n/, $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated (doubled)');

# 16-19: BUG -- quadruple the test data
push @TEST_FILES, @TEST_FILES;
is( $#TEST_FILES+1, 8, 'Sanity check: there are 8 test files' );
$expected = `cat @TEST_FILES`;
$cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not corrupt concatenated http messages (quads)');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages (quads)');
@stderr_lines = split /\n/, $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated (quads)');

