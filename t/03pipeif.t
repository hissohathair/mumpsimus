#!/usr/bin/env perl

use warnings;
use strict;

use lib './t/lib';

use Test::Command tests => 3;
use Test::More;

BEGIN {
    $ENV{PATH} = './src:.:' . $ENV{PATH};
}


my $TEST_FILE = './t/test-data/sample-response-302.txt';

my $cmd = Test::Command->new( cmd => q{pipeif -h -c "sed -e 's/^Server:.*$/Serviette: banana/' "} . " < $TEST_FILE"  );

$cmd->exit_is_num( 0, 'Command exited normally' );

$cmd->builder->todo_start( 'Under construction' );
$cmd->stdout_unlike( qr/^Server: gws$/ms, 'The existing Server header should be stripped' );
$cmd->stdout_like( qr/^Serviette: banana$/ms, 'A new Server header has been inserted'  );
$cmd->builder->todo_end();

