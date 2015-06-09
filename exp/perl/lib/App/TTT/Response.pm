# TTT::Response
#
# $Id$ $Revision$
# $Source$ $Author$ $Date$
#
# Contains common functions and things for handling HTTP responses on stdin and stdout.
# 
#
package App::TTT::Response;
use base qw(App::TTT::Message HTTP::Response);

# Pragma
use warnings;
use strict;

# Modules
use autodie;
use IO::Handle;
use Carp qw(croak);

## no critic
our ($VERSION) = ( '\$Revision: 1 \$' =~ m{ \$Revision: \s+ (\S+) }msx );
## use critic


sub read_from_fh
{
    my ($self, $fh, $str, @rest) = @_;    
    if ( !defined $str ) {
	$str =  '';
    }

    # First line ought to be a request line
    $str = <$fh> unless ( $str );
    chomp($str);
    if ( $str =~ m{^(HTTP/\d+\.\d+)? \s* ([\d]+) \s+ (.*)$}sxm ) {
	# New response statusline resets the object
	$self->clear();

	# Set the request line
	$self->protocol( $1 );
	$self->code( $2 );
	$self->message( $3 );

	# Read the rest of the message
	$self->SUPER::read_from_fh( $fh, @rest );
    }
    else {
	die "read_from_fh: Did not start with HTTP request line but \"$str\"";
    }

    return;
}

sub status_line
{
    my $self = shift @_ or croak "Expected self";
    return join(' ', $self->protocol, $self->SUPER::status_line) . "\n";
}


sub headline
{
    my ($self) = @_;
    return $self->status_line();
}
