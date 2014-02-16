################################################################################
#
# Copyright (c) 2014 SirAnthony <anthony at adsorbtion.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
################################################################################

use strict;
use warnings;
use Getopt::Std;
use genTools;
use genSettings;

our $opt_p;
getopt('p');
require "$opt_p/glheaders.pm";

our @api;
our @debuggableDrawCalls;
our @shaderSwitches;
our @frameEndMarkers;
our @framebufferChanges;
my %debuggableDrawCallsMap = map { $_->[0] => ($_->[1] + 1) } @debuggableDrawCalls;
my %shaderSwitchesMap = map { $_ => 1 } @shaderSwitches;
my %frameEndMarkersMap = map { $_ => 1 } @frameEndMarkers;
my %framebufferChangesMap = map { $_ => 1 } @framebufferChanges;
my $WIN32 = ($^O =~ /Win32/);


sub createListEntry
{
	my ($extname, $fname) = @_;
	my $prefix = (split("_", $extname, 2))[0];

	return sprintf
	"{\"$prefix\", \"$extname\", \"$fname\", %s, %s, %s, %s, %s},",
		($debuggableDrawCallsMap{$fname} or 0),
		(($debuggableDrawCallsMap{$fname} or 0) - 1),
		($shaderSwitchesMap{$fname} or 0),
		($frameEndMarkersMap{$fname} or 0),
		($framebufferChangesMap{$fname} or 0);
}

sub out
{
	my $input = shift;
	my @elements;
	foreach my $func (@$input) {
		push @elements, createListEntry($func->[1], $func->[3]);
	}

	printf '
#include <stdlib.h>
#include "debuglib.h"
GLFunctionList glFunctions[] = {
	%s
	{NULL, NULL, NULL, 0, -1, 0, 0}
};
', join "\n\t", @elements;
}

header_generated();
out(\@api);

