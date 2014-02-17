################################################################################
#
# Copyright (c) 2014 SirAnthony <anthony at adsorbtion.org>
# Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
# (VIS), Universit?t Stuttgart.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice, this
#    list of conditions and the following disclaimer in the documentation and/or
#    other materials provided with the distribution.
#
#  * Neither the name of the name of VIS, Universitдt Stuttgart nor the names
#    of its contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
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
our %files;
our %regexps;

our ($opt_p, $opt_m);
getopt('pm');
require "$opt_p/glheaders.pm";
our @api;

my @modes = ("decl", "def", "exp");
if (not grep(/^$opt_m$/, @modes)) {
	die "Argument must be one of " . join(", ", @modes) . "\n";
}

my @initializer = ();
my @extinitializer = ();
my @functions = ();


sub defines {
	my $mode = shift;
	if ($mode eq "decl") {
		print "#ifndef __TRAMPOLINES_H
#define __TRAMPOLINES_H
#pragma once

/* needed for DebugFunctions to know where the functions are located */
#ifdef glsldebug_EXPORTS
#define DEBUGLIBAPI __declspec(dllexport)
#else /* glsldebug_EXPORTS */
#define DEBUGLIBAPI __declspec(dllimport)
#endif /* glsldebug_EXPORTS */

";
	} elsif ($mode eq "exp") {
		print "LIBRARY \"glsldebug\"
EXPORTS
";
	}
}

sub footer {
	my $mode = shift;
	if ($mode eq "def") {
		printf qq|
#include "functionRefs.inc"
		
void initTrampolines() {
%s
}

void initExtensionTrampolines() {
%s
}
|, join("\n", @initializer), join("\n", @extinitializer);

		print qq|

int attachTrampolines() {
	int i;
	initTrampolines();
	for (i = 0; i < FUNC_REFS_COUNT; ++i) {
		/* Do not attach extension functions, it will be loaded with wglGetProcAddress */
		if (refs_ExtFuncs[i])
			continue;
		dbgPrint(DBGLVL_DEBUG, "Attaching %s 0x%x\\n", refs_FuncsNames[i], *refs_OrigFuncs[i]);
		if (!Mhook_SetHook(refs_OrigFuncs[i], refs_HookedFuncs[i])) {
			dbgPrint(DBGLVL_DEBUG, "Mhook_SetHook(%s) failed.\\n", refs_FuncsNames[i]);
			return 0;
		}
	}
	return 1;
}

int detachTrampolines() {
	int i;
	for (i = 0; i < FUNC_REFS_COUNT; ++i) {
		/* Extensions was not attached */
		if (refs_ExtFuncs[i])
			continue;
		if (!Mhook_Unhook(&((PVOID)refs_OrigFuncs[i]))) {
			dbgPrint(DBGLVL_DEBUG, "Mhook_Unhook(%s) failed.\\n", refs_FuncsNames[i]);
			return 0;
		}
	}
	return 1;
}

|;
	} elsif ($mode eq "decl") {
		print "void initExtensionTrampolines();
int attachTrampolines();
int detachTrampolines();
#endif /* __TRAMPOLINES_H */
";
	}

	print "\n";
}

sub addFunction
{
	my ($isExtension, $retval, $fname, $argList) = @_;

	if ($isExtension){
		push @extinitializer, "	Orig$fname = ($retval (APIENTRYP)($argList)) OrigwglGetProcAddress(\"$fname\");";
	} else {
		push @initializer, "	Orig$fname = $fname;
	dbgPrint(DBGLVL_DEBUG, \"Orig$fname = 0x%x\\n\", $fname);";
		push(@functions, $fname);
	}
}

sub createTrampoline
{
	my ($mode, $isExtension, $extname, $retval, $fname, $argString) = @_;
	my @arguments = buildArgumentList($argString);
	my $argList = join(", ", @arguments);
	my $ret = "	Orig$fname";

	if ($mode eq "def") {
		$ret = "$retval (APIENTRYP Orig$fname)($argList) = NULL;
/* Forward declaration: */ __declspec(dllexport) $retval APIENTRY Hooked$fname($argList);";
		addFunction($isExtension, $retval, $fname, $argList);
	} elsif ($mode eq "decl") {
		$ret = "extern DEBUGLIBAPI $retval (APIENTRYP Orig$fname)($argList);";
	}

	return $ret;
}

# Begin output
header_generated($opt_m eq "exp" ? ";" : "//");
defines($opt_m);
foreach my $apidef (@api) {
	print createTrampoline($opt_m, @$apidef) . "\n";
}
footer($opt_m);
