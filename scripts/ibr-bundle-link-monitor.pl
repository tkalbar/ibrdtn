#!/usr/bin/perl

# a little script to monitor the number and size of bundles
#
# now it's enhanced to keep track of open links too. - bdw
#
# this script is more efficient (and a little more careless)
# than the old one
# It relies on 'bundle stats' and 'storage usage' to get its data
# The drawback is that we can't distinguish between different
# types of bundles this way


use IO::Socket;
use FileHandle;
use Getopt::Std;
use strict;

if (@ARGV==0) {
    print("usage: $0 [options]\n");
    print("[-l logfile]\n[-r sample_interval]\n");
    exit(0);
}


my ($filename, $log, $sample_interval);

our($opt_l, $opt_r);
getopts("l:r:");

if ($opt_l) {
    $filename = $opt_l;
    open($log, ">$filename") or die "ERROR: unable to open $filename\n";
    #$log = LOG;
} else {
    die "ERROR1\n";
    #$log = STDOUT;
}
$log->autoflush(1);

if ($opt_r) {
    $sample_interval = $opt_r;
} else {
    $sample_interval = 10;
}

while (1) {
    my $sock = new IO::Socket::INET (
	PeerAddr => 'localhost',
	PeerPort => '4550',
	Proto => 'tcp',
	);
    # We don't actually want to die if we couldn't open the socket
    # Just try again in 10 seconds (or $sample_interval)
    #die "Could not create socket: $!\n" unless $sock;

    if ($sock) {
        print $sock "protocol management\n";

        print $sock "neighbor list\n";
        my $done = 0;
        my $num_nbrs = 0;
        # ignore first 3 lines
        my $line = <$sock>;
        my $line = <$sock>;
        my $line = <$sock>;
        
        while (! $done) {
            my $line = <$sock>;
            if ($line =~ /dtn/) {
                $num_nbrs++;
            } else {
	        $done = 1;
            }
        }
        print "num neighbors: ".$num_nbrs."\n";

        print $sock "bundle list\n";
        my $num_bundles = 0;
        # ignore first line
        my $line = <$sock>;
        
        while (! $done) {
            my $line = <$sock>;
            if ($line =~ /dtn/) {
                $num_bundles++;
            } else {
	        $done = 1;
            }
        }
        print "num bundles: ".$num_bundles."\n";
	#print($log time()."\t$num_nbrs\t$num_bundles\n");
	print(time()."\t$num_nbrs\t$num_bundles\n");

        close($sock);
    }
    sleep($sample_interval);
}

close($log);
