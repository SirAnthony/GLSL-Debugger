################################################################################
#
# Copyright (c) 2013, 2014 SirAnthony <anthony at adsorbtion.org>
# Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
# (VIS), Universität Stuttgart.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
#   * Neither the name of the name of VIS, Universität Stuttgart nor the names
#   of its contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
################################################################################

use strict;
use warnings;
use Getopt::Std;
require genTypes;
require genTools;
our %regexps;
our $opt_p;
getopt('p');

require "$opt_p/glheaders.pm";
our @api;
our @pfn;

my $WIN32 = ($^O =~ /Win32/);
my %defined_types = ();

sub print_type
{
	my $retval = shift;
	my $pfname = shift;
	printf  "\ntypedef $retval (APIENTRYP $pfname)(%s);",
		join(", ", map { $_ } @_);
}

sub createFPType
{
	my ($retval, $fname, $argString) = @_;
	return if $defined_types{$fname};

	$retval =~ s/^\s+|\s+$//g;
	my @arguments = buildArgumentList($argString);
	print_type($retval, "PFN${fname}PROC", @arguments);
	$defined_types{$fname} = 1;
}


header_generated();

# Add PFN definitions first
foreach my $pfndef (@pfn) {
	#$defined_types{uc($pfndef->[2])} = 1;
}

# Add missing functions
foreach my $apidef (@api) {
	# Uppercase then lowercase variants
	createFPType($apidef->[2], uc($apidef->[3]), $apidef->[4]);
	createFPType($apidef->[2], $apidef->[3], $apidef->[4]);
}

print "\n";

