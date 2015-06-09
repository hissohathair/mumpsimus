# TTT.pm
#
# $Id$ $Revision$
# $Source$ $Author$ $Date$
#
# Contains common functions and things for handling HTTP requests on stdin and stdout.
# 
#
package App::TTT::Request;
use base qw(App::TTT::Message HTTP::Request);

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




# Common browser HTTP request headers
our %browser_headers = (
    'firefox' => { 
	'User-Agent'      => 'Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.5; en-US; rv:1.9.0.1) Gecko/2008070206 Firefox/3.0.1',
	'Accept'          => 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
	'Accept-Charset'  => 'ISO-8859-1,utf-8;q=0.7,*;q=0.7',
	'Accept-Language' => 'en-us,en;q=0.5',
	'Accept-Encoding' => 'gzip, deflate',
    },

    'msie' => {
      'Accept'            => 'image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-ms-application, application/x-ms-xbap, application/vnd.ms-xpsdocument, application/xaml+xml, */*',
      'Accept-Language'   => 'en-au',
      'UA-CPU'            => 'x86',
      'Accept-Encoding'   => 'gzip, deflate',
      'User-Agent'        => 'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; InfoPath.1; MS-RTC LM 8; .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)',
    },

    );



# set_browser
#
sub set_browser
{
    my $self    = shift @_ or croak "Expected self";
    my $browser = shift @_ or croak "Expected browser name";

    if ( defined $browser_headers{$browser} ) {
	foreach my $k ( keys %{ $browser_headers{$browser} } ) {
	    $self->header( $k => $browser_headers{$browser}->{$k} );
	}

    } 
    else {
	die "get_browser_headers: Do not know the browser '$browser'";
    }

    return;
}



sub read_from_fh
{
    my ($self, $fh, $str, @rest) = @_;
    if ( !defined $str ) {
	$str = '';
    }

    # First line ought to be a request line
    $str = <$fh> unless ( $str ); 
    chomp($str);

    if ( $str =~ m{^([A-Z]+) \s+ ([^\s]+) \s+ (HTTP/\d+\.\d+)\s*$}sxm ) {
	# New request line resets the object
	$self->clear();

	# Set the request line
	$self->method( $1 );
	$self->uri( $2 );
	$self->protocol( $3 );

	# Read the rest of the message
	$self->SUPER::read_from_fh( $fh, @rest );
    }
    else {
	die "read_from_fh: Did not start with HTTP request line but \"$str\"";
    }

    return;
}


sub request_line
{
    my $self = shift @_ or croak "Expected self";
    return join(' ', $self->method, $self->uri, $self->protocol) . "\n";
}


sub headline
{
    my ($self) = @_;
    return $self->request_line();
}
