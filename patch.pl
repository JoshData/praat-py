#!/usr/bin/perl

# This program patches a file by looking for a line
# matching a regex and inserting a line after it.

$file = shift(@ARGV);

open IN, "<$file";
$in = join("", <IN>);
close IN;

while (scalar(@ARGV)) {
	$find = shift(@ARGV);
	$repl = shift(@ARGV);
	$repl =~ s/\\t/\t/g;
	$repl =~ s/\\n/\n/g;

	$repl2 = $repl;
	$repl2 = quotemeta($repl2);
	
	if ($in !~ s/(\n[^\n]*$find[^\n]*\n)($repl2\n)*/$1$repl\n/) {
		print STDERR "Could not find: $find\n";
		exit(1);
	}
}

open OUT, ">$file";
print OUT $in;
close OUT;
