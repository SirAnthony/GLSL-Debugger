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

require genTools;
our %regexps;

my $WIN32 = ($^O =~ /Win32/);
my ($api, $typedef, $pfn, $defines) = ([], [], [], []);
my ($apimap, $typedefmap, $pfnmap, $definesmap) = ({}, {}, {}, {});

sub push_to_array
{
	my ($arrayr, $mapr, $refnum) = @_;
	return sub {
		push @{$arrayr}, [map { $_ } @_] if not $mapr{@_[$refnum]}++;
	}
}

sub print_array
{
	my ($name, $format, $array) = @_;
	printf "our $name = (\n%s
);\n", join ",\n", map { sprintf $format, @$_ } @$array;
}

my @arrays = (
	[$api, $apimap, 3, "glapi", "wingdi", "winapifunc", "glxfunc"],
	[$typedef, $typedefmap, 3, "typegl", "typewgl", "typeglx"],
	[$pfn, $pfnmap, 2, "pfn"],
	[$defines, $definesmap, 2, "glvar", "wglvar", "glxvar"]
);

my %output = (
	'@api' => ['["%s", "%s", "%s", "%s", "%s"]', $api],
	'@typedefs' => ['["%s", "%s", "%s", "%s"]', $typedef],
	'@pfn' => ['["%s", "%s", "%s"]', $pfn],
	'@defines' => ['["%s", "%s", "%s"]', $defines],
);

header_generated("#");
my @actions;
foreach my $type (@arrays){
	my $sub = push_to_array(splice @$type, 0, 3);
	foreach my $regexp (@{$type}){
		push @actions, [$regexps{$regexp}, $sub];
	}
};

parse_gl_files(\@actions, $WIN32, push_to_array($api));
map { print_array($_, @{$output{$_}}) } keys %output;
