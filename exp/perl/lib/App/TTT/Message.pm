# TTT::Message
#
# $Id$ $Revision$
# $Source$ $Author$ $Date$
#
# Contains common functions and things for handling HTTP messages on stdin and stdout.
# Normally, you want TTT::Request or TTT::Response.
# 
#
package App::TTT::Message;
use base 'HTTP::Message';

# Pragma
use warnings;
use strict;

# Modules
use autodie;
use IO::Handle;

## no critic
our ($VERSION) = ( '\$Revision: 1 \$' =~ m{ \$Revision: \s+ (\S+) }msx );
## use critic

use constant BUFFER_SIZE => 512;



sub read_from_fh
{
    my $self      = shift @_ or croak "Expected self";
    my $fh        = shift @_ or croak "Expected file handle";
    my $head_only = shift @_ || 0;


    # Headers are easy -- read from the file handle until you come across
    # a blank line -- that's the end of the header. Everything after that is the
    # request body.
    #

    my $line = 0;
    while ( <$fh> ) {
	chomp;
	$line++;

	if ( m{^([\w-]+): \s+ (.*)$}sxmi ) {
	    $self->header( $1 => $2 );
	}
	elsif ( m{^[\n\r]*$}sxm ) {
 	    last;
	}
	else {
	    warn "headers: Could not parse header line $line\n";
	}	
    }


    if ( $head_only ) {
	return;
    }


    # We're out of the header part but is there a request body to read?
    #
    my $con_len = $self->header( 'content_length' ) or 0;
    if ( $con_len ) {
	my $content = '';
	read $fh, $content, $con_len
	    or warn "read: Failed to read $con_len bytes ($!)\n";
	$self->content( $content );
    }

    return;
}


sub pass_body_through
{
    my ( $self, $show_body ) = @_;

    my $body_bytes_read = 0;
    my $bytes_to_read = $self->header('Content-Length') || 0;

    while ( $body_bytes_read < $bytes_to_read ) {
	my $content = '';
	my $bytes_read = read *STDIN, $content, BUFFER_SIZE;
	if ( !defined $bytes_read ) {
	    warn "read: Error reading from stdin ($!)\n";
	    last;
	}
	elsif ( $bytes_read <= 0 ) {
	    last; # TODO: why am I doing this?
	}
	else {
	    $body_bytes_read += $bytes_read;

	    if ( $show_body ) {
		print $content;
	    }
	}
    }
    return;
}



sub get_special_headers
{
    my $self = shift @_ or croak "Expected self";

    my @all_headers = $self->header_field_names;
    my %special_headers = ( );
    foreach my $h ( @all_headers ) {
	next unless ( $h =~ m/^X-TTT-/i );
	$special_headers{$h} = $self->header( $h );
    }
    return %special_headers;
}


sub clear_special_headers
{
    my $self = shift @_ or croak "Expected self";

    my @all_headers = $self->header_field_names;
    foreach my $h ( @all_headers ) {
	next unless ( $h =~ m/^X-TTT-/i );
	$self->remove_header( $h );
    }
    return ;
}


sub set_special_headers
{
    my $self = shift @_ or croak "Expected self";
    my %headers = @_;

    foreach my $k ( keys %headers ) {
	$k = "X-TTT-" . ucfirst(lc($k)) unless ( $k =~ m/^X-TTT-/i );
	$self->header( $k => $headers{$k} ) ;
    }
    return;
}


sub headline
{
    my ($self) = @_;
    croak "headline: Pure virtual function called";
}
