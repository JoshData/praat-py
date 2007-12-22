#!/usr/bin/perl

# This program patches a file by looking for a line
# matching a regex and inserting a line after it, or
# replacing that line.

if ($ARGV[0] eq "--replace") {
	$replace = 1;
	shift(@ARGV);
}

$file = shift(@ARGV);

open IN, "<$file";
$in = join("", <IN>);
close IN;

while (scalar(@ARGV)) {
	$find = shift(@ARGV);
	$repl = shift(@ARGV);
	
	$find = quotemeta($find);
	
	$repl =~ s/\\t/\t/g;
	$repl =~ s/\\n/\n/g;

	$repl2 = $repl;
	$repl2 = quotemeta($repl2);
	
	if ($in !~ s/(\n[ \t]*)($find *\n)($repl2\n)*/$1 . (!$replace ? $2 : '') . $repl . "\n"/e) {
		print STDERR "Could not find: $find\n";
		exit(1);
	}
}

if (!-e "$file.bak") {
	system("mv $file $file.bak");
}

open OUT, ">$file";
print OUT $in;
close OUT;
