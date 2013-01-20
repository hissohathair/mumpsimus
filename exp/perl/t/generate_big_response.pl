#!/usr/bin/perl -w
#
#

use warnings;
use strict;

use App::TTT;

use Time::HiRes qw( usleep );

use HTTP::Status qw( :constants status_message );
use HTTP::Date;
use HTTP::Response;

use constant DEFAULT_LENGTH => 1024;
use constant HTML_HEAD      => '<html><head><title>Response</title></head><body>';
use constant HTML_FOOT      => '</body></html>';
use constant BYTES_PER_SEC  => 100;


my $length = shift @ARGV || DEFAULT_LENGTH;
my $usecs_per_byte = (1 / BYTES_PER_SEC) * 1000000;


my $res = HTTP::Response->new( );
$res->code( HTTP_OK );
$res->protocol( 'HTTP/1.1' );
$res->message( status_message(HTTP_OK)  );
$res->header( 
    server         => $0,
    content_type   => 'text/html',
    date           => time2str(time),
    content_length => $length,
    );


# Account for header and footer 
$length -= length HTML_HEAD . HTML_FOOT;
if ( $length < 0 ) {
    $length = 0;
    warn "$0: Length was too small -- autocorrecting\n";
}


# Only prints header since that's all we have so far
print $res->as_string;
print HTML_HEAD;

while ( $length > 0 ) {
    print q{.};
    usleep($usecs_per_byte);
    $length--;
}

print HTML_FOOT;
