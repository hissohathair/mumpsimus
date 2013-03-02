#!/usr/bin/env perl
#

use warnings;
use strict;

my @TESTS = @ARGV;
if ( $#TESTS < 0 ) {
    push @TESTS, "./system-tests/*.t" if ( -d './system-tests' );
    push @TESTS, '*.t' if ( -f 'noop.t' );
}

my $rc = 0;
if ( &prove_exists() ) {
    $rc = system("prove @TESTS");
}
else {
    foreach my $testf ( @TESTS ) {
	$rc += system("perl $testf");
    }
}


exit($rc);


sub prove_exists 
{
    my @path_elts = split /:/, $ENV{PATH};
    foreach my $p ( @path_elts ) {
	if ( -e "$p/prove" ) {
	    return "$p/prove";
	}
    }
    return 0;
}
