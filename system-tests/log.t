#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 16;
use Test::More;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
}


my @TEST_FILES = ( 'sample-request-get.txt', 'sample-response-302.txt' );
my @SEARCH_DIR = ( 'test-data', 'system-tests/test-data' );
for ( my $i = 0; $i <= $#TEST_FILES; $i++ ) {
    my $path = `find @SEARCH_DIR -name $TEST_FILES[$i] 2>/dev/null`;
    chomp($path);
    #diag("Test file $i at $path");
    $TEST_FILES[$i] = $path if ( $path );
}

# 1-8: simple requests 
foreach my $testf ( @TEST_FILES ) {
    my $fsize = -s $testf;
    my $cmd = Test::Command->new( cmd => "log < '$testf'" );
    $cmd->stdout_is_file($testf, "log does not molest single http message from $testf");
    $cmd->stderr_like(qr/^log.c: \[(\w\w\w)\]/, "log message was generated from $testf");
    $cmd->stderr_like(qr/\($fsize bytes\)/, "log message counted $fsize bytes correctly");
    $cmd->exit_is_num(0, "log exited with zero during $testf");
}

# 9-10: request + response
my $expected = `cat @TEST_FILES`;
my $cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not molest concatentated http messages');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages');

# 11: num log messages == num http messages
my @stderr_lines = split /\n/, $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated');

# 12-15: double the test data
my $orig_files = $#TEST_FILES+1;
push @TEST_FILES, @TEST_FILES;
is( $#TEST_FILES+1, $orig_files * 2, 'we doubled our test files' );
$expected = `cat @TEST_FILES`;
$cmd = Test::Command->new( cmd => "cat @TEST_FILES | log" );
$cmd->stdout_is_eq($expected, 'log does not molest concatenated http messages (doubled)');
$cmd->exit_is_num(0, 'log exited with zero for concatenated http messages (doubled)');
@stderr_lines = split /\n/, $cmd->stderr_value;
is( $#stderr_lines+1, $#TEST_FILES+1, '1 log message for each http message generated (doubled)');

# 16: BUG -- zero bytes being reported
$cmd = Test::Command->new( cmd => 'log < ' . $TEST_FILES[0] );
$cmd->stderr_is_eq( "log.c: [req] GET http://www.google.com/ HTTP/1.1 (527 bytes)\n", 'Counted 527 bytes correctly' );
