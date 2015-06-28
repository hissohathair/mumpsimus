#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 40;
use Test::More;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
}

my $COMMAND = 'body';
my $TRANSFORM = "tr '[:lower:]' '[:upper:]'";

my @SEARCH_DIR = ( 'test-data', 'system-tests/test-data' );

my $HEAD_TEST_FILE = qx{ find @SEARCH_DIR -name sample-request-get.txt 2>/dev/null };   chomp($HEAD_TEST_FILE);
my $BODY_TEST_FILE = qx{ find @SEARCH_DIR -name sample-response-302.txt 2>/dev/null };  chomp($BODY_TEST_FILE);
my @TEST_FILES = ( $HEAD_TEST_FILE, $BODY_TEST_FILE );



# 1-6: Headers left alone test (test file has header only)
my $expected_req = qx{ cat $HEAD_TEST_FILE | egrep -v '^X-Mumpsimus' };
my $cmd = Test::Command->new( cmd => qq{ $COMMAND -c "$TRANSFORM" < $HEAD_TEST_FILE  }  );
stress_test($cmd, 'HTTP request TR test', $expected_req);

# 7-12: Headers left alone (test file has response body)
my $expected_res = qx{ cat $BODY_TEST_FILE };
if ( $expected_res =~ m/(.*)\r\n\r\n(.*)/sg ) {
    $expected_res = $1 . "\r\n\r\n" . uc($2);
}

$cmd = Test::Command->new( cmd => qq{ $COMMAND -c "$TRANSFORM" < $BODY_TEST_FILE | egrep -v '^X-Mumpsimus' } );
stress_test($cmd, 'HTTP response TR test', $expected_res);

# 13-18: Now testing HTTP streams (request, response)
my $expected_noop = qx{ cat @TEST_FILES }; 

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c noop | egrep -v '^X-Mumpsimus'} );
stress_test($cmd, 'HTTP req/res NOOP test', $expected_noop);

# 19-24: Now test a longer HTTP stream (req, res, req, res)
push @TEST_FILES, @TEST_FILES;
$expected_noop = qx{ cat @TEST_FILES };

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c noop | egrep -v '^X-Mumpsimus'} );
stress_test($cmd, 'HTTP req/res x 2 NOOP test', $expected_noop);


# 25-30: Now test the longer HTTP stream with a body transform
my $expected = $expected_req . $expected_res . $expected_req . $expected_res;

$cmd = Test::Command->new( cmd => qq{ cat @TEST_FILES | $COMMAND -c "$TRANSFORM" | egrep -v '^X-Mumpsimus'} );
stress_test($cmd, 'HTTP req/res x 2 TR test', $expected);


# 31-32: Test for a body transformation that LENGTHENS the body
$expected_res = qx{ cat $BODY_TEST_FILE };
my ($expected_length, $old_length);
if ( $expected_res =~ m/(.*)\r\n\r\n(.*)/sg ) {
    my ($head, $body) = ($1, $2);
    $old_length = length($body);
    $body =~ s/moved/moves babe/;
    $expected_length = length($body);
}

$cmd = Test::Command->new( cmd => qq{ cat $BODY_TEST_FILE | $COMMAND -c "sed 's/moved/moves babe/'" } );
like( $cmd->stdout_value, qr/^Content-Length: $expected_length/m, "Content-Length updated correctly to $expected_length" );
like( $cmd->stdout_value, qr/^X-Mumpsimus-Original-Content-Length: $old_length/m, "Original-Length reported as $old_length" );


# 33-34: Test for a body transformation that SHORTENS the body
$expected_res = qx{ cat $BODY_TEST_FILE };
if ( $expected_res =~ m/(.*)\r\n\r\n(.*)/sg ) {
    my ($head, $body) = ($1, $2);
    $old_length = length($body);
    $body =~ s/moved//;
    $expected_length = length($body);
}

$cmd = Test::Command->new( cmd => qq{ cat $BODY_TEST_FILE | $COMMAND -c "sed 's/moved//'" } );
like( $cmd->stdout_value, qr/^Content-Length: $expected_length/m, "Content-Length updated correctly to $expected_length" );
like( $cmd->stdout_value, qr/^X-Mumpsimus-Original-Content-Length: $old_length/m, "Original-Length reported as $old_length" );


# 35-40: Non-matching Content-Type should not be filtered
$expected = qx{ cat $BODY_TEST_FILE };
$cmd = Test::Command->new( cmd => qq{ cat $BODY_TEST_FILE | $COMMAND -c "$TRANSFORM" -t xxx-no-such-type } );
stress_test($cmd, 'body tool does not filter when content-type does not match', $expected);



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
