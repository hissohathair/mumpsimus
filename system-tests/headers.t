#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 30;
use Test::More;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
}

my $COMMAND = 'headers';

my @SEARCH_DIR = ( 'test-data', 'system-tests/test-data' );

my $HEAD_TEST_FILE = qx{ find @SEARCH_DIR -name sample-request-get.txt 2>/dev/null };   chomp($HEAD_TEST_FILE);
my $BODY_TEST_FILE = qx{ find @SEARCH_DIR -name sample-response-302.txt 2>/dev/null };  chomp($BODY_TEST_FILE);
my @TEST_FILES = ( $HEAD_TEST_FILE, $BODY_TEST_FILE );



# 1-6: Header transformation test (test file has header only)
my $expected = qx{ cat $HEAD_TEST_FILE };
$expected =~ s/^DNT: 1/DNT: banana/gm; 

my $cmd = Test::Command->new( cmd => qq{ $COMMAND -c "sed -e 's/^DNT: 1/DNT: banana/'" < $HEAD_TEST_FILE }  );
stress_test($cmd, 'HTTP request SED test', $expected);


# 7-12: Header transformation test (test file has response body)
$expected = qx{ cat $BODY_TEST_FILE };
$expected =~ s/^Server: gws/Server: banana/gm;

$cmd = Test::Command->new( cmd => qq{ $COMMAND -c "sed -e 's/^Server: gws/Server: banana/'" < $BODY_TEST_FILE } );
stress_test($cmd, 'HTTP response SED test', $expected);


# 13-18: Now testing HTTP streams (request, response)
$expected = qx{ cat @TEST_FILES }; 

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c noop } );
stress_test($cmd, 'HTTP req/res NOOP test', $expected);


# 19-24: Now test a longer HTTP stream (req, res, req, res)
push @TEST_FILES, @TEST_FILES;
$expected = qx{ cat @TEST_FILES };

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c noop } );
stress_test($cmd, 'HTTP req/res x 2 NOOP test', $expected);


# 25-30: Now test the longer HTTP stream with a header transform (sed)
$expected =~ s/^Server: gws/Server: banana/gm;

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c "sed -e 's/^Server: gws/Server: banana/'" } );
stress_test($cmd, 'HTTP req/res x 2 SED test', $expected);



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
    $cmd->exit_is_num( 0,   "$label: command exited normally" );
    $cmd->stderr_is_eq( '', "$label: no error messages on stderr" );
    $cmd->stdout_is_eq( $expected, "$label: stdout was expected" );

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
    is( $errors{stdout_val}, 0, "$label: output as expected $max_test_runs times" );

    return;
}
