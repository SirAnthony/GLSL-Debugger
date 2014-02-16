
use strict;
use warnings;
use Getopt::Std;
our $opt_p;
getopt('p');
require "$opt_p/glheaders.pm";
our @api;

my %records;
foreach my $apirec (@api) {
    $records{uc($apirec->[3])} = $apirec->[3];
}

my %out;
my $ifh;
open $ifh, "<", "../streamRecording.h" or die $!;
while (<$ifh>) {
    chomp;

    if (/#define DBG_STREAM_HINT_(\S+) (\S+)/) {
        my $status =  ($2 eq "DBG_RECORD_AND_REPLAY") ? 1 :
                      ($2 eq "DBG_RECORD_AND_FINAL") ? 2 : 0;
        next if not $status;
        die "no func for $1" unless $records{$1};
        $out{$records{$1}} = $status;
    }
}
close $ifh;

print qq|
\@stream_hints_types = (
"STREAMHINT_NO_RECORD",
"STREAMHINT_RECORD_AND_REPLAY",
"STREAMHINT_RECORD_AND_FINAL",
);
|;

print qq|
\%stream_hints = (
|;
while (my ($key, $val) = each %out) {
    print "\"$key\" => $val,
";
}
print ");
";
