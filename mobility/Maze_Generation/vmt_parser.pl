#!/usr/bin/env perl
use strict;
use warnings;

my $infile = $ARGV[0];
my $outfile = $ARGV[1];

#Initialization variables
my $units = "meters";
my $dimension = "2";
my $model = "freespace";
my $gps = "41.925247,-73.31183";

my $desc = "mobile";
my $range = "1000";



open(my $in,  "<",  $infile)  or die "Can't open input.txt: $!";
open(my $out, ">",  $outfile) or die "Can't open output.txt: $!";

print $out "<MESHTEST> \n";


my $current_time = 0;

while (<$in>){
	chomp;

	if ($. == 1 or $. ==2) {
	}
	else {

		(my $time, my $node, my $x, my $y, my $speed, my $direction, my $angle, my $z) = split("\t");
		#$node = $node +1;  # Fix indexing

		if ($time == $current_time){
			print $out "\t\t<NODE INPUT=\"$node\" X=\"$x\" Y=\"$y\" DESC=\"$desc\" RANGE=\"$range\" />\n";
		}
		else{
			if ($current_time == 0){
				print $out "\t<PHYSICAL TIME=\"$time\" UNITS=\"$units\" DIMENSION=\"$dimension\" MODEL=\"$model\" GPS_ORIGIN=\"$gps\">\n";
				print $out "\t\t<NODE INPUT=\"$node\" X=\"$x\" Y=\"$y\" DESC=\"$desc\" RANGE=\"$range\" />\n";
				$current_time = $time;
			}
			else{
			print $out "\t</PHYSICAL>\n";
			print $out "\t<PHYSICAL TIME=\"$time\" UNITS=\"$units\" DIMENSION=\"$dimension\" MODEL=\"$model\" GPS_ORIGIN=\"$gps\">\n";
			print $out "\t\t<NODE INPUT=\"$node\" X=\"$x\" Y=\"$y\" DESC=\"$desc\" RANGE=\"$range\" />\n";
			$current_time = $time;
			}
		}

	}

}

close ($in);
close ($out);
