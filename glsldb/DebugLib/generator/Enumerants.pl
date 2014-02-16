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
our %extnames_defines;
our @problem_defines;
our $opt_p;
getopt('p');

my %problem_defines_map = map {$_ => 1} @problem_defines;


sub generate_element
{
	my $name = shift;
	my $out = "\t{$name, \"$name\"},";
	if ($problem_defines_map{$name}) {
		$out = "#ifdef $name
$out
#endif /* $name */";
	}
	return $out;
}

sub check_define {
	my ($extname, $elements) = @_;
	my $out = join("\n", map generate_element($_), @$elements);
	if ($extname) {
		$out = "
#ifdef $extname
$out
#endif /* $extname */
";
	}
	return $out;
}

sub out_struct {
	my ($name, $order, $enums) = @_;
	my $elements = "";
	foreach my $extname (@$order){
		next if not defined $enums->{$extname};
		$elements .= check_define($extname, $enums->{$extname});
	}
	print "
static EnumerantsMap ${name}[] = {
$elements
	{0, NULL}
};
";

}

sub out
{
	my ($order, $enums) = @_;
	foreach my $struct_name (keys %$enums) {
		out_struct($struct_name, $order, $enums->{$struct_name});
	}
}

sub convert
{
	my $input = shift;
	my $enumerants = {};
	my $extenumerants = {};
	my $bits = {};
	my $keys_order = [];

	foreach my $define (@$input) {
		my ($isExtension, $ext, $enum) = @$define;
		next if $enum =~ /GL_FALSE|GL_TRUE|GL_TIMEOUT_IGNORED/;

		$ext = $extnames_defines{$ext} if defined $extnames_defines{$ext};
		push @$keys_order, $ext unless grep{$_ eq $ext} @$keys_order;

		# TODO: there is BIT enums in GLX, do we need them as extBitfield?
		my $array = ($enum =~ m/^(?:GLX|WGL)_/ ) ? $extenumerants :
					($enum =~ /_BIT$|_BIT_\w+$|_ATTRIB_BITS/) ? $bits :
						$enumerants;
		$array->{$ext} = [] unless $array->{$ext};
		push @{$array->{$ext}}, $enum;
	}

	return ($keys_order, {
		"glEnumerantsMap" => $enumerants,
		"extEnumerantsMap" => $extenumerants,
		"glBitfieldMap" => $bits
	});
}

require "$opt_p/glheaders.pm";
our @defines;

header_generated();
print "typedef struct {
	GLenum value;
	const char *string;
} EnumerantsMap;
";

out(convert(\@defines));
