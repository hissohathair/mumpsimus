#!/usr/bin/env perl

use warnings;
use strict;

use lib './lib';

use Test::Command tests => 5;
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
    diag("Test file $i at $path");
    $TEST_FILES[$i] = $path if ( $path );
}

my $HEAD_TEST_FILE = $TEST_FILES[0];
my $BODY_TEST_FILE = $TEST_FILES[1];

# 1: Header transformation test (test file has header only)
my $cmd = Test::Command->new( cmd => q{pipeif -h -c "sed -e 's/^DNT:.*$/DNT: banana/'" < } . $HEAD_TEST_FILE  );
$cmd->exit_is_num( 0, 'Command exited normally' );
if ( $cmd->stdout_value =~ m/DNT: ([\w]+)/s ) {
    my $server = $1;
    is( $server, 'banana', 'DNT header was changed to banana' );
}
else {
    fail( 'DNT header was completely missing' );
}

# 2: Check that rest of header was preserved OK
my $expected = `cat $HEAD_TEST_FILE`;
$expected =~ s/^Server: gws$/Server: banana/s; 
$cmd->stdout_is_eq( $expected, 'Rest of headers were preserved OK' );

# 3: Body transformation (with noop -- so no change really). Mainly making sure header/body order preserved
$cmd = Test::Command->new( cmd => q{pipeif -b -c noop < } . $BODY_TEST_FILE );
$expected = `cat $BODY_TEST_FILE`;
$cmd->exit_is_num( 0, 'No-op test exited normally' );
$cmd->stderr_is_eq( '', 'No error messages on stderr' );
$cmd->stdout_is_eq( $expected, 'Piping through noop was genuine no-op' );

### TODO: Under Construction -- expected to fail
$cmd->builder->todo_start( 'Under construction' );
$cmd->builder->todo_end();
### END TODO

