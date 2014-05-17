#!/usr/bin/perl 

use strict;
use warnings;
use FileHandle;
use Sys::Hostname;
use DBI;
use List::Util qw[min max];

sub usage {
    print "usage: $0 <radius> <overlap> <width> <height> <prob_missing> <speed> <pause> <duration> <seed>\n";
    exit(0);
}

if (@ARGV != 9) {
    usage();
}

my ($radius, $overlap, $width, $height, $p_missing, $speed, $pause, $duration, $seed) = @ARGV;

srand($seed);

my $PI = 3.1415;

my @x = ();
my @y = ();
my @cx = ();
my @cy = ();
my %node_index = ();
my @node_location = ();
my @phase = ();
my $num_nodes = 0;
my $angular_speed = $speed/(2.0*$PI*$radius);
my $offset = 2.0*$radius;
#my ($gpsx,$gpsy) = (0.0,0);

my $n = 0;
for (my $i=0; $i<$width; $i++) {
    for (my $j=0; $j<$height; $j++) {
	$cx[$n] = $offset + $radius*2*$i;
	$cy[$n] = $offset + $radius*2*$j;
	$node_index{"$i:$j"} = $n;
	$node_location[$n] = "$i:$j";
	$phase[$n] = (($i+$j)%2)*$PI;
	$n++;
	$num_nodes++;
    }
}

print "<MESHTEST>\n";

for (my $t=0; $t<$duration; $t++) {
    print "  <PHYSICAL TIME=\"$t\" UNITS=\"meters\" DIMENSION=\"2\" "
	."MODEL=\"freespace\" GPS_ORIGIN=\"41.925247,-73.31183\">\n";

    for (my $i=0; $i<$num_nodes; $i++) {
	$x[$i] = $cx[$i] + $radius * cos($phase[$i] + $t*$angular_speed);
	$y[$i] = $cy[$i] + $radius * sin($phase[$i] + $t*$angular_speed);

	print "    <NODE INPUT=\"".($i+1)."\" X=\"".$x[$i]."\" Y=\"".$y[$i]."\" DESC=\"mobile\" RANGE=\"1000\" ";
	print "/>\n";
    }
    print "  </PHYSICAL>\n";
}
print "</MESHTEST>\n";


