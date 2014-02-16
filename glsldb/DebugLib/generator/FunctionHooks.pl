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
use prePostExecuteList;
use genTools;
use genTypes;
use streamHints;

our $opt_p;
getopt('p');
require "$opt_p/functionsAllowedInBeginEnd.pm";
require "$opt_p/glheaders.pm";

our @api;
our @typedefs;
our %typeMap;
our %stream_hints;
our @allowedInBeginEnd;
my %beginEnd = map {$_ => 1} @allowedInBeginEnd;
my $WIN32 = ($^O =~ /Win32/);


sub print_defines
{
	printf 'include "streamHintRefs.inc"';

	if (defined $WIN32)	{
		print "#define ENTER_CS EnterCriticalSection
#define EXIT_CS LeaveCriticalSection
#define RECURSING(n) rec->isRecursing = n;
";
	} else {
		print "#define ENTER_CS pthread_mutex_lock
#define EXIT_CS pthread_mutex_unlock
#define RECURSING(n)
";
	}

	print "
int check_error(int check_allowed, int dummy)
{
	if (!dummy && (!check_allowed || G.errorCheckAllowed))
		return ORIG_GL(glGetError)();
	return GL_NO_ERROR;
}

int check_error_code(int error, int do_stop)
{
	if (error != GL_NO_ERROR) {
		setErrorCode(error);
		if (do_stop)
			stop();
	} else {
		return 1;
	}
	return 0;
}
";
}

sub check_error_string
{
	my ($check, $void, $fname) = @_;
	# never check error after glBegin
	return sprintf "check_error(%i, %i)",
		int(defined $beginEnd{$fname}),
		($check and (not $void or $fname ne "glBegin")) ? 0 : 1;
}



#==============================================
#   READING BELOW THIS LINE MAY BE HARMFUL
#               STOP HERE
#==============================================


sub check_error_store
{
	my ($check, $void, $fname, $postexec, $return_type, $indent) = (@_);
	return "" if $void and not $check;

	my $ret = "";
	my ($storeResult, $storeResultErr) = ("", "");
	$indent = "    " x $indent;
	$postexec = "\n" . $postexec if $postexec;

	if (not $void){
		$storeResult = "storeResult(&result, $return_type);";
		$storeResultErr = "storeResultOrError(error, &result, $return_type);";
	}

	if ($check) {
		if ($fname eq "glBegin") {
			# never check error after glBegin
			$ret = "error = GL_NO_ERROR;${postexec}";
		}elsif (not $beginEnd{$fname}) {
			$ret = "error = ORIG_GL(glGetError)();${postexec}${storeResultErr}";
		} else {
			$ret = "if (G.errorCheckAllowed) {
	error = ORIG_GL(glGetError)();${postexec}${storeResultErr}
} else {
	error = GL_NO_ERROR;${postexec}${storeResult}
}";
		}
		$ret = "$ret
setErrorCode(error);" if $void;
	} else {
		$ret = "error = GL_NO_ERROR;${postexec}${storeResult}";
	}


	$ret =~ s/^/$indent/g;
	$ret =~ s/\n/\n$indent/g;
	return $ret;
}

sub thread_statement
{
	my ($fname, $retval, $retval_assign, $return_name, @arguments) = (@_);
	my $statement;
	if (defined $WIN32) {
		my $preexec = pre_execute($fname, @arguments);
		my $postexec = post_execute($fname, $retval, @arguments);
		my $argstring = arguments_string(@arguments);

		my $check_allowed = "";
		$check_allowed = "G.errorCheckAllowed = 1;\n" if $fname eq "glEnd";
		$check_allowed = "G.errorCheckAllowed = 0;\n" if $fname eq "glBegin";
		$statement = "
	DbgRec *rec;
	dbgPrint(DBGLVL_DEBUG, \"entering $fname\\n\");
	rec = getThreadRecord(GetCurrentProcessId());
	${check_allowed}if(rec->isRecursing) {
		dbgPrint(DBGLVL_DEBUG, \"stopping recursion\\n\");
		${preexec}${retval_assign}ORIG_GL($fname)($argstring);
		/* no way to check errors in recursive calls! */
		error = GL_NO_ERROR;
		${postexec}return $return_name;
	}
	rec->isRecursing = 1;";
	}
	return $statement;
}


# TODO: check position of unlock statements!!!
my %defined_funcs = (
	"wglGetProcAddress" => 1,	# Hooked
);

# OH GOSH, rewrite it, please.

sub createBody
{
	my ($fnum, $retval, $fname, $argString, $checkError) = @_;
	$fname =~ s/^\s+|\s+$//g;
	$retval =~ s/^\s+|\s+$//g;
	return if $defined_funcs{$fname}++;

	my @arguments = buildArgumentList($argString);
	my $pfname = join("","PFN",uc($fname),"PROC");

	my $return_void = $retval =~ /^void$|^$/i;
	my $return_type = $return_void ? "void" : getTypeId($retval);
	my $return_name =  $return_void ? "" : "result";
	my $retval_assign = $return_void ? "" : "${return_name} = ";
	# temp. variable that holds the return value of the function
	my $retval_init = $return_void ? "" : "${retval} ${return_name};\n    ";

	my $argstring = arguments_string(@arguments);
	my $argrefstring = arguments_references(@arguments);
	my $argsizes = arguments_sizes($fname, @arguments);
	$argsizes = ", " . $argsizes if $argsizes;

	my $argcount = scalar @arguments;
	my $argtypes = ($arguments[0] !~ /^void$|^$/i) ?
		join("", map { ", &arg$_, " . getTypeId(
			$arguments[$_]) } (0..$#arguments)) : "";

	my $ucfname = uc($fname);
	my $win_recursing = defined $WIN32 ? "rec->isRecursing = 0;\n" : "";
	my $preexec = pre_execute($fname, @arguments);
	my $postexec = post_execute($fname, $retval, @arguments);

	my $thread_statement = thread_statement($fname,
					$retval, $retval_assign, $return_name, @arguments);
	print $fname;
	my $errstr = check_error_string($checkError, $return_void, $fname);

	my $output = "";
	###########################################################################
	# create function head
	###########################################################################
	if (defined $WIN32) {
		$output .= "__declspec(dllexport) $retval APIENTRY Hooked$fname (";
	} else {
		$output .= "$retval $fname (";
	}

	# add arguments to function head
	if ($arguments[0] =~ /^void$|^$/i){
		$output .= "void"
	} else {
		$output .= join(", ", map { ($arguments[$_] =~ /(.*)(\[\d+\])/ ?
				"$1 arg$_$2" : ($arguments[$_] . " arg$_"))
				} (0..$#arguments));
	}

	my $streamhint = "";
	if ($stream_hints{$fname}) {
		$streamhint = qq|
			if (refs_StreamHint[$fnum] != STREAMHINT_NO_RECORD)
				recordFunctionCall(&G.recordedStream, "$fname", ${argcount}${argsizes});
			if (refs_StreamHint[$fnum] == STREAMHINT_RECORD_AND_FINAL)
				break;|;
	}


	###########################################################################
	# first store name of called function and its arguments in the shared memory
	# segment, then call dbgFunctionCall that stops the process/thread and waits
	# for the debugger to handle the actual debugging
	$output .= ")
{
	${retval_init}int op, error;${thread_statement}
	ENTER_CS(&G.lock);
	if (keepExecuting(\"$fname\")) {
		EXIT_CS(&G.lock);
		${preexec}${retval_assign}ORIG_GL($fname)($argstring);
		error = GL_NO_ERROR;
		if (checkGLErrorInExecution())
			error = ${errstr};
		${postexec}if (check_error_code(error, 1)){
			RECURSING(0)
			return  $return_name;
		}
	}
	//fprintf(stderr, \"ThreadID: %li\\n\", (unsigned long)pthread_self());
	storeFunctionCall(\"$fname\", ${argcount}${argtypes});
	stop();
	op = getDbgOperation();
	while (op != DBG_DONE) {
		switch (op) {
		case DBG_CALL_FUNCTION:
			${retval_assign}(($pfname)getDbgFunction())($argstring);";
	if (not $return_void) {
		$output .= "
			storeResult(&result, $return_type);";
	}
	$output .= sprintf "
			break;
		case DBG_RECORD_CALL:$streamhint
		case DBG_CALL_ORIGFUNCTION:
			${preexec}${retval_assign}ORIG_GL($fname)($argstring);
%s
			break;
		case DBG_EXECUTE:
			setExecuting();
			stop();
			EXIT_CS(&G.lock);
			${preexec}${win_recursing}${retval_assign}ORIG_GL($fname)($argstring);
			error = GL_NO_ERROR;
			if (checkGLErrorInExecution())
				error = ${errstr};
			${postexec}if (check_error_code(error, 0)){
				RECURSING(0)
				return  $return_name;
			}
		default:
			executeDefaultDbgOperation(op);
		}
		stop();
		op = getDbgOperation();
	}
	setErrorCode(DBG_NO_ERROR);
	EXIT_CS(&G.lock);
	${win_recursing}return $return_name;
}

", check_error_store($checkError, $return_void, $fname, $postexec, $return_type, 3);

	$output =~ s/return ;/return;/g;
	print $output;
}


foreach my $typedef (@typedefs) {
	addTypeMapping(@$typedef);
}

header_generated();
print_defines();
for my $i (0..$#api) {
	my $check_error = (@{$api[$i]}[1] =~ /^GLX_/) ? 0 : 1;
	createBody($i, @{$api[$i]}[2..4], $check_error);
}
