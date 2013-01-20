# TTT.pm
#
# Contains common functions and things for handling HTTP requests on stdin and stdout.
# 
# This module makes stdout unbuffered which you'll need to do to get pipes running
# correctly.
#

package App::TTT;

# Pragma
use warnings;
use strict;

# Modules
use autodie;
use IO::Handle;
use File::Spec;


## no critic
our ($VERSION) = ( '1.0' );
## use critic


# This module Exports only a few things by default.
#
use base 'Exporter';
our @EXPORT    = qw( HTTP_PORT );
our @EXPORT_OK = qw( give_me_a_message_or_give_me_death get_script_location );


# Constants and stuff.
#
use constant HTTP_PORT => 80;


# Will almost always need unbuffered IO on stdout
#
*STDOUT->autoflush();


sub give_me_a_message_or_give_me_death
{
    my $str       = shift @_ || croak "Expected string";
    my $head_only = shift @_ || 0;

    my $mess = undef;
    if ( $str =~ m{^([A-Z]+) \s+ ([^\s]+) \s+ (HTTP/\d+\.\d+)\s*$}sxm ) {
	$mess = App::TTT::Request->new();
	$mess->read_from_fh( *STDIN, $_, $head_only );
    }
    elsif ( $str =~ m{^(HTTP/\d+\.\d+)? \s* ([\d]+) \s+ (.*)\s*$}sxm ) {
	$mess = App::TTT::Response->new();
	$mess->read_from_fh( *STDIN, $_, $head_only );
    } 
    else {
	die "Did not recognise string as a HTTP request OR response";
    }

    return $mess;
}


sub get_script_location
{
    my $script_name = shift @_ or croak "Expected a script name";
    my $directory   = shift @_ || File::Spec->curdir();

    my $new_location = $script_name;
    if ( -f $directory ) {
	my ($vol, $directories, $file) = File::Spec->splitpath( $directory );
	$new_location = File::Spec->catpath($vol, $directories, $script_name);
    } 
    else {
	$new_location = File::Spec->catfile($directory, $script_name);
    }

    return $new_location;
}


1;    # End of App::TTT

__END__

=head1 NAME

App::TTT - A container for common functions in the Teeny-Tiny-Toolkit.

=head1 VERSION

Version 0.1

=head1 SYNOPSIS

Etc.

=head1 DESCRIPTION

Etc.

=head1 SUBROUTINES/METHODS

Etc.

=head1 DIAGNOSTICS

Etc.

=head1 CONFIGURATION AND ENVIRONMENT

Etc.

=head1 DEPENDENCIES

Etc.

=head1 INCOMPATIBILITIES

Etc.

=head1 BUGS AND LIMITATIONS

Etc.

=head1 AUTHOR

Daniel Austin <daniel.austin@gmail.com>

=head1 LICENSE AND COPYRIGHT

Etc.


