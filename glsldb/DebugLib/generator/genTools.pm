################################################################################
#
# Copyright (c) 2013 SirAnthony <anthony at adsorbtion.org>
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

use Time::localtime;
require genSettings;
our %extname_matches;
our %files;
our @skip_defines;
our @force_extensions;
my %skip_defines_map = map { $_ => 1 } @skip_defines;
my %force_extensions_map = map { $_ => 1 } @force_extensions;

our %regexps = (
    "glapi" => qr/^\s*(?:GLAPI\b)\s+(.*?)\s*(?:GL)?APIENTRY\s+(.*?)\s*\(\s*(.*?)\s*\)/,
    "wingdi" => qr/^\s*(?:WINGDIAPI\b)\s+(.*?)\s*(?:WINAPI|APIENTRY)\s+(wgl.*?)\s*\((.*?)\)/,
    "winapifunc" => qr/^\s*(?!WINGDIAPI)(.*?)\s*WINAPI\s+(wgl\S+)\s*\((.*)\)\s*;/,
    "glxfunc" => qr/^\s*(?:GLAPI\b|extern\b)\s+(\S.*\S)\s*(glX\S+)\s*\((.*)\)\s*;/,
    "typegl" => qr/^\s*typedef\s+(.*?)\s*(GL\w+)\s*;/,
    "typewgl" => qr/^\s*typedef\s+(.*?)\s*(WGL\w+)\s*;/,
    "typeglx" => qr/^\s*typedef\s+(.*?)\s*(GLX\w+)\s*;/,
    "pfn" => qr/^\s*typedef.*\((?:GL)?APIENTRY\S*\s+PFN(\S+)PROC\)/,
    "glvar" => qr/^\s*#define\s+(GL_\w+)\s+0x[0-9A-Fa-f]*/,
    "glxvar" => qr/^\s*#define\s+(GLX_\w+)\s+0x[0-9A-Fa-f]*/,
    "wglvar" => qr/^\s*#define\s+((WGL|ERROR)_\w+)\s+0x[0-9A-Fa-f]*/,
);


# I wanted to make it clear
# Well, shit...
sub parse_output {
    my ($filename, $bextname, $api, $actions) = (@_);
    my $extname = $bextname;
    my @definitions = ($extname);
    my $proto = $extname;
    my $api_re = qr/^#ifndef\s+($api\S+)/;
    my $isNative = grep { $$_[0] eq $filename } values %files;
    my @matched_defines;
    my $current_define = 0;
    my $last_define = 0;

    my $ifh;
    my $is_stdin = 0;
    if ($filename) {
        open $ifh, "<", $filename or die $!;
    } else {
        $ifh = *STDIN;
        $is_stdin++;
    }

    while (<$ifh>) {
        chomp;

        # Skip comments
        next if /^\/\//;

        # Multiline function
        if (/^\s*(?:GLAPI\b|WINGDIAPI\b|extern\b)\s+\S.*\S\s*\([^)]*?$/) {
            my $fprototype = $_;
            chomp $fprototype;
            while ($fprototype !~ /.*;\s*$/) {
                my $line = <$ifh>;
                chomp $line;
                $line =~ s/\s*/ /;
                $fprototype .= $line;
            }
            $_ = $fprototype;
        }

        # Prototype name in #define or in comment
        my $reg_match = $extname_matches{$_};
        if ($reg_match || ($current_define eq $last_define &&
                /^#define\s+$extname\s+1/)) {
            if ($reg_match){
                push @definitions, $extname;
                $extname = $reg_match;
            }
            $proto = $extname;
        }

        # Run each supplied regexp here
        if ($current_define eq $last_define) {
            my $isExtension = ($force_extensions_map{$extname} or !$isNative);
            foreach my $group (@$actions){
                my ($regexp, $func) = @$group;
                if (my @matches = /$regexp/){
                    $func->($isExtension, $proto, @matches);
                }
            }
        }

        # Exit from selection
        if (/^#endif/ and (scalar @matched_defines)) {
            my $endif = pop @matched_defines;
            next if $endif and $skip_defines_map{$endif};
			# ugh
			for my $define (reverse @matched_defines){
				$last_define = $define;
				last if not $skip_defines_map{$define};
			}
			
            if ($endif =~ $api and $endif eq $current_define){
                $current_define = $last_define;
                $extname = pop @definitions;
                $proto = $bextname;
            }
        }

        # Enter selection
        if (/^#if/) {
            my $ifdef = /^#ifn?def\s+(\S+)/;
            push @matched_defines, $1;
            $current_define = $1 if !$current_define;
            next if not $ifdef or $skip_defines_map{$1};
            $last_define = $1;
            if (/$api_re/) {
                push @definitions, $extname;
                $extname = $1;
                $current_define = $last_define;
            }
        }
    }
    close $ifh unless $is_stdin;
}


sub header_generated
{
    my $style = shift;
    my $t = localtime;
    my $year = $t->year + 1900;
    my $header = sprintf "////////////////////////////////////////////////////////
//
//   THIS FILE IS GENERATED AUTOMATICALLY %02d.%02d.%04d %02d:%02d:%02d
//
// Copyright (c) %04d Perl generator
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the \"Software\"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////

", $t->mday, $t->mon + 1, $year, $t->hour, $t->min, $t->sec, $year;
    $header =~ s|^//|$style|mg if $style;
    print $header;
}


sub parse_gl_files {
    my $gl_actions = shift;
    my $WIN32 = shift;
    my $win32func = shift;
    my @params = ([$files{"gl"}, "GL_VERSION_1_0", qr/GL_/, $gl_actions]);

    if ($WIN32) {
        push @params, [$files{"wgl"}, "WGL_VERSION_1_0",
                        qr/WGL_/, $gl_actions];
        # Additional function from original file
        $win32func->(0, "WGL_VERSION_1_0", "BOOL", "SwapBuffers",
                        "HDC") if $win32func;
    } else {
        push @params, [$files{"glx"}, "GLX_VERSION_1_0",
                        qr/GLX_/, $gl_actions];
    }

    foreach my $entry (@params) {
        my $filenames = shift @$entry;
        foreach my $filename (@$filenames) {
            parse_output($filename, @$entry);
        }
    }
}

