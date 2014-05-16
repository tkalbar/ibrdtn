#!/usr/bin/perl

use strict;

if (@ARGV !=2) {
    die "usage: $0 <infile> <outbase>\n";
}

my ($infile, $outbase) = @ARGV;

my $FH;
open($FH,"<$infile") || die "ERROR: could not open $infile for reading\n";

my $geo_file_count = 0;
my $hop_file_count = 0;
my $OFH;
my $HFH;
my $ofilename = "$outbase-geo-$geo_file_count.dat";
$geo_file_count = $geo_file_count+1;
open($OFH,">$ofilename") || die "ERROR: could not open $ofilename for writing\n";

my $lasthop = "";
my $thishop = "";
my $lastlat = 0;
my $lastlon = 0;
my $lasttimestamp = 0;
my $pending_hopdata = 0;

while (my $line = <$FH>) {
    chomp($line);
    if ($line =~ /\(HOPDATA \, (\d+) , (\S+)\)/) {
	my ($timestamp, $eid) = ($1,$2);
	#print "read a hopdata: $1 $2\n";

	close($OFH);
	my $ofilename = "$outbase-geo-$geo_file_count.dat";
	$geo_file_count = $geo_file_count+1;
	open($OFH,">$ofilename") || die "ERROR: could not open $ofilename for writing\n";

	if ($lasthop ne "") {
	    $thishop = $eid;
	    $pending_hopdata = 1;
	} else {
	    $lasthop = $eid;
	}
	$lasttimestamp = $timestamp;
    }

    if ($line =~ /\(GEODATA \, (\d+) , \((\S+) , (\S+)\)\)/) {
	my ($timestamp,$lat,$lon) = ($1,$2,$3);
	#print "read a geodata: $1 $2 $3\n";
	#print $OFH "$timestamp\t$lat\t$lon\t\"$thishop\"\n";
	print $OFH "$timestamp\t$lat\t$lon\n";

	if ($pending_hopdata == 1) {
	    if (($lastlat!=0) && ($lastlon!=0)) {
		$pending_hopdata = 0;
		my $HFH;
		my $hopfilename = "$outbase-hop-$hop_file_count.dat";
		$hop_file_count = $hop_file_count+1;
		open($HFH, ">$hopfilename")
		    || die "ERROR: could not open $hopfilename for writing\n";
		#print $HFH "$lasttimestamp\t$lastlat\t$lastlon\t$lasthop\n";
		#print $HFH "$timestamp\t$lat\t$lon\t$thishop\n";
		print $HFH "$lasttimestamp\t$lastlat\t$lastlon\n";
		print $HFH "$timestamp\t$lat\t$lon\n";
		close($HFH);
	    }
	}
	$lasthop = $thishop;
	$lastlat = $lat;
	$lastlon = $lon;
    }
}

close($OFH);

close($FH);

