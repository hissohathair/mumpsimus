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
    $rc = system_check("prove @TESTS");
}
else {
    foreach my $testf ( @TESTS ) {
	$rc += system_check("perl $testf");
    }
}

exit($rc);


sub prove_exists 
{
    my @path_elts = split /:/, $ENV{PATH};    # :/; emacs you can't parse for shit'
    foreach my $p ( @path_elts ) {
	if ( -e "$p/prove" ) {
	    warn "Using Prove as test harness (found in $p)\n";
	    return "$p/prove";
	}
    }
    warn "Using Perl as test driver (cannot find prove)\n";
    return 0;
}


sub system_check
{
    my $rc = system( @_ );
    if ($rc == -1) {
        warn "system: failed to execute ($!)\n";
    }
    elsif ($rc & 127) {
        warn "system: child died with signal " . ($? & 127) . "\n";
    }
    else {
        $rc = $rc >> 8;
    }
    return $rc;
}
