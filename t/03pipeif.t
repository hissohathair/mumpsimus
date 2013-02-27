#!/usr/bin/env perl

use warnings;
use strict;

use lib './t/lib';

use Test::Command tests => 5;
use Test::More;

BEGIN {
    $ENV{PATH} = './src:.:' . $ENV{PATH};
    $ENV{ULOG_LEVEL} = 4;
}


my $TEST_FILE = './t/test-data/sample-response-302.txt';

my $cmd = Test::Command->new( cmd => q{pipeif -h -c "sed -e 's/^Server:.*$/Server: banana/' " < } . $TEST_FILE  );

$cmd->exit_is_num( 0, 'Command exited normally' );

if ( $cmd->stdout_value =~ m/Server: ([\w]+)/s ) {
    my $server = $1;
    is( $server, 'banana', 'Server name was changed to banana' );
}
else {
    fail( 'Server header was completely missing' );
}

$cmd = Test::Command->new( cmd => q{pipeif -b -c noop < } . $TEST_FILE );
my $expected = `cat $TEST_FILE`;
$cmd->exit_is_num( 0, 'noop test exited normally' );
$cmd->stderr_is_eq( '', 'no error messages on stderr' );

### TODO: Under Construction -- expected to fail
$cmd->builder->todo_start( 'Under construction' );
$cmd->stdout_is_eq( $expected, 'piping through noop was genuine no-op' );
$cmd->builder->todo_end();
### END TODO

