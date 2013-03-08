#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib', './system-tests/lib';

use Test::Command tests => 21;
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

my $HEAD_TEST_FILE = $TEST_FILES[0];
my $BODY_TEST_FILE = $TEST_FILES[1];

# 1-2: Header transformation test (test file has header only)
my $cmd = Test::Command->new( cmd => q{pipeif -h -c "sed -e 's/^DNT: 1/DNT: banana/'" < } . $HEAD_TEST_FILE  );
$cmd->exit_is_num( 0, 'Command exited normally' );
if ( $cmd->stdout_value =~ m/DNT: ([\w]+)/s ) {
    my $server = $1;
    is( $server, 'banana', 'DNT header was changed to "banana"' );
}
else {
    fail( 'DNT header was completely missing' );
}

# 3: Check that rest of header was preserved OK
my $expected = `cat $HEAD_TEST_FILE`;
$expected =~ s/^DNT: 1/DNT: banana/gm; 
$cmd->stdout_is_eq( $expected, 'Rest of headers were preserved OK' );

# 4-6: Body transformation (with noop -- so no change really). Mainly making sure header/body order preserved
$cmd = Test::Command->new( cmd => q{pipeif -b -c noop < } . $BODY_TEST_FILE );
$expected = `cat $BODY_TEST_FILE`;
stress_test($cmd, "Body Test", $expected);

# 10-12: Header transformation (again with noop), this time send headers through pipe
$cmd = Test::Command->new( cmd => q{pipeif -h -c noop < } . $BODY_TEST_FILE );
$expected = `cat $BODY_TEST_FILE`;
stress_test($cmd, "Head Test", $expected);

# 16-18: Header AND body transformation (again with noop), this time send headers through pipe
$cmd = Test::Command->new( cmd => q{pipeif -hb -c noop < } . $BODY_TEST_FILE );
$expected = `cat $BODY_TEST_FILE`;
stress_test($cmd, "Head+Body Test", $expected);

### TODO: Under Construction -- expected to fail
$cmd->builder->todo_start( 'Under construction' );
$cmd->builder->todo_end();
### END TODO


sub stress_test
{
    my ($cmd, $label, $expected) = @_;
    my $max_test_runs = 100;

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
    is( $errors{stdout_val}, 0, "$label: output unmolested correctly $max_test_runs times" );

    return;
}
