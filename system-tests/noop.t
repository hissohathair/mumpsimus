#!/usr/bin/env perl

use warnings;
use strict;

use Test::More tests => 7;
use Test::Command;

BEGIN {
    $ENV{PATH} = '../src:./src:' . $ENV{PATH};
}


# 1-2: noop does nothing to piped input
my $expected = `cat Makefile`;
my $actual   = `cat Makefile | noop`;
ok( length($expected) > 100, 'We appear to have some test data :)' );
is( $actual, $expected,   'noop does not modify output' );

# 3: null totally consumes piped input
$actual = `cat Makefile | null`;
is( $actual, '', 'null consumes all output' );

# 4: dup says everything twice says everything twice
$actual = `echo one | dup 2>&1`;
$expected = "one\none\n";
is( $actual, $expected, 'dup says everything twice' );

# 5: More thorough test of dup
$expected = `cat Makefile`;
my $cmd = Test::Command->new( cmd => q{ cat Makefile | dup } );
$cmd->exit_is_num(0, 'dup command exited normally');
$cmd->stdout_is_eq($expected, 'dup did not modify stdout');
$cmd->stderr_is_eq($expected, 'dup copied to stderr');
