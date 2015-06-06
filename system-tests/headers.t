#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 6;
use Test::More;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
}

my $COMMAND = 'headers';

my @SEARCH_DIR = ( 'test-data', 'system-tests/test-data' );

my $HEAD_TEST_FILE = qx{ find @SEARCH_DIR -name sample-request-get.txt 2>/dev/null };
my $BODY_TEST_FILE = qx{ find @SEARCH_DIR -name sample-response-302.txt 2>/dev/null };
my @TEST_FILES = ( $HEAD_TEST_FILE, $BODY_TEST_FILE );



# 1-3: Header transformation test (test file has header only)
my $expected = `cat $HEAD_TEST_FILE`; 
$expected =~ s/^DNT: 1/DNT: banana/gm; 

my $cmd = Test::Command->new( cmd => qq{ $COMMAND -c "sed -e 's/^DNT: 1/DNT: banana/'" < $HEAD_TEST_FILE }  );
$cmd->exit_is_num( 0, 'HTTP request test command exited normally' );
$cmd->stdout_is_eq( $expected, 'HTTP request test headers were expected' );
$cmd->stderr_is_eq( '', 'HTTP request test had no errors on stderr' );


# 4-6: Header transformation test (test file has response body)
$expected = `cat $BODY_TEST_FILE`;
$expected =~ s/^Server: gws/Server: banana/gm;

$cmd = Test::Command->new( cmd => qq{ $COMMAND -c "sed -e 's/^Server: gws/Server: banana/'" < $BODY_TEST_FILE } );
$cmd->exit_is_num( 0, 'HTTP response test command exited normally' );
$cmd->stdout_is_eq( $expected, 'HTTP response test headers were expected' );
$cmd->stderr_is_eq( '', 'HTTP response test headers had no errors on stderr' );


# stress_test: 6 tests
#
#    For the $cmd given, run the command 11 times, testing each time
#    that the exit status is zero, no output appears on stderr, and
#    the output on stdout matches $expected. Uses $label to report
#    results.
#
#    This produces 6 tests for the whole set (1 x 3 smoke tests; plus
#    1 x 3 load tests).
#
sub stress_test
{
    my ($cmd, $label, $expected) = @_;
    my $max_test_runs = 10;

    # Smoke test
    $cmd->run();
    $cmd->exit_is_num( 0,   "$label: no-op test exited normally" );
    $cmd->stderr_is_eq( '', "$label: no error messages on stderr" );
    $cmd->stdout_is_eq( $expected, "$label: noop was genuine no-op" );

    # Stress test
    my %errors = ( exit_val => 0, stdout_val => 0, stderr_val => 0, );
    for ( my $i = 0; $i < $max_test_runs; $i++ ) {
	$cmd->run();
	$errors{exit_val}++   if ( $cmd->exit_value != 0 );
	$errors{stderr_val}++ if ( $cmd->stderr_value ne '' );
	$errors{stdout_val}++ if ( $cmd->stdout_value ne $expected );
    }
    is( $errors{exit_val},   0, "$label: command exited normally $max_test_runs times" );
    is( $errors{stderr_val}, 0, "$label: command generated 0 errors $max_test_runs times" );
    is( $errors{stdout_val}, 0, "$label: output uncorrupted correctly $max_test_runs times" );

    return;
}
