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
use streamHints;

our %stream_hints;
our @stream_hints_types;
our $opt_p;
getopt('p');

my $line_length = 10;

require "$opt_p/glheaders.pm";
our @api;

sub min ($$) { $_[$_[0] > $_[1]] }

sub createRefs
{
	my @functions = @_;
	my @groups;
	my ($count, $start) = (scalar $#functions, 0);
	while ($start <= $count) {
		push @groups, join(", ",
			 map { $stream_hints_types[$stream_hints{$_} or 0] }
			@functions[$start..(min($start+$line_length, $count)-1)]);
		$start += $line_length;
	}

	$count += 2;
	printf "
#define STREAMHINTS_REFS_COUNT $count
enum StreamHints {
%s
};

enum StreamHints refs_StreamHint[STREAMHINTS_REFS_COUNT] = {
%s
};
", join(",\n", @stream_hints_types),
   join(",\n", @groups);
}

header_generated();
createRefs(map { $_->[3] } @api);
