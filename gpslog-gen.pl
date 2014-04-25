#!/usr/bin/perl

use strict;

if (@ARGV != 2) {
    die "usage:\n\t $0 <number of entries> <interval>\n";
}

my ($n_entries, $interval) = @ARGV;

my $t = time;
my ($lat,$lon);

for (my $i=0; $i<$n_entries; $i++) {
    $lat = 10.0 * $t/1398368725;
    $lon = 20.0 * $t/1348868725;
    print "$t;$lat;$lon\n";
    $t += $interval;
}
