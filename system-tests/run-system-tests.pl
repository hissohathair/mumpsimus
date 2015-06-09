#!/usr/bin/env perl
#
# usage:
#   $ run-system-tests.pl [test_file.t [test_file2.t [test_fileN.t ...]]]
#
# run-system-tests.pl: 
#
#   Test harness for system testing. With no arguments, will run all
#   the tests to run a specific test file (or files) list them on the
#   command line.
#
#   If the 'prove' command is detected it will be used to run the
#   tests.  Otherwise we call Perl directly.
#

use warnings;
use strict;


# Check command line for arguments. If there are none, add all *.t
# files.
#
my @TESTS = @ARGV;
if ( $#TESTS < 0 ) {
    if ( -d './system-tests' ) {
	@TESTS = split /\n+/, qx{ ls -1 ./system-tests/*.t } ;
    }
    elsif ( -f 'noop.t' ) {
	@TESTS = split /\n+/, qx{ ls -1 *.t } ;
    }
}
chomp(@TESTS);
warn "Found " . ($#TESTS+1) . " tests: @TESTS\n";

# Return code ($rc) will be the exit code of the called
# program. Non-zero indicates failed tests, which we need to
# propogate.
#
my $rc = 0;
if ( &prove_exists() ) {
    $rc = system_check("prove @TESTS");
}
else {
    foreach my $testf ( @TESTS ) {
	warn "Running: $testf\n";
	$rc += system_check("perl $testf");
    }
}

if ( $rc ) {
    warn "Test ERRORS detected (rc=$rc)\n";
}
else {
    warn "Tests appear to have run OK\n";
}
exit($rc);


# prove_exists:
#
#    Return true if the command 'prove' is found.
#
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


# system_check :
#
#   Call the system(2) function, check for abnormal terminations, and
#   return the correct exit code of the child program.
#
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
