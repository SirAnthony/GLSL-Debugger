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
our $opt_p;
getopt('p');

my $WIN32 = ($^O =~ /Win32/);
my $line_length = 10;

require "$opt_p/glheaders.pm";
our @api;

sub min ($$) { $_[$_[0] > $_[1]] }

sub createRefs
{
	my @functions = @_;
	my (@orig, @hooked, @names);
	my $count = scalar @functions;
	my $type = (defined $WIN32) ? "PVOID" : "void";

	my $start = 0;
	while ($start < $count) {
		my @group = @functions[$start..min($start+$line_length, $count-1)];
		push @names, join ", ", map { "\"$_\"" } @group;
		if ($WIN32) {
			push @orig, join ", ", map { "&((PVOID)Orig$_)" } @group;
			push @hooked, join ", ", map { "Hooked$_" } @group;
		} else {
			push @orig, join ", ", map { "(void*)$_" } @group;
		}
		$start += $line_length;
	}

	printf "#define FUNC_REFS_COUNT $count
$type* refs_OrigFuncs[FUNC_REFS_COUNT] = {
%s
};
const char* refs_FuncsNames[FUNC_REFS_COUNT] = {
%s
};
", join(",\n", @orig), join(",\n", @names);

	if ($WIN32) {
		printf "$type refs_HookedFuncs[FUNC_REFS_COUNT] = {
%s
};
", join(",\n", @hooked);
	}

}

header_generated();
createRefs(map { $_->[3] } @api);
