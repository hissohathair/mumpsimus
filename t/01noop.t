#!/usr/bin/env perl

use warnings;
use strict;

use Test::More tests => 4;

BEGIN {
    $ENV{PATH} = './src:.:' . $ENV{PATH};
}


# 1-2: noop does nothing to piped input
my $expected = `cat Makefile`;
my $actual   = `cat Makefile | noop`;
ok( length($expected) > 100, 'We appear to have some test data :)' );
is( $actual, $expected,   'noop does not modify output' );

# 3: null totally consumes piped input
$actual = `cat Makefile | null`;
is( $actual, '', 'null consumes all output' );

# 4: upcase converts all letters to upper case (assumes Perl behaviour is same)
$expected = uc $expected;
$actual = `cat Makefile | upcase`;
is( $actual, $expected, 'upcase converts to upper case' );


