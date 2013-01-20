# TTT::UserAgent
#
# $Id$ $Revision$
# $Source$ $Author$ $Date$
#
# A derivative of LWP::UserAgent to be used for fetching stuff.
# 
#
package App::TTT::UserAgent;
use base qw(LWP::UserAgent);

# Pragma
use warnings;
use strict;

# Modules
use autodie;

## no critic
our ($VERSION) = ( '\$Revision: 1 \$' =~ m{ \$Revision: \s+ (\S+) }msx );
## use critic


1;
