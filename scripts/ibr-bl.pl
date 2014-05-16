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


my $sock = new IO::Socket::INET (
    PeerAddr => 'localhost',
    PeerPort => '4550',
    Proto => 'tcp',
);
die "Could not create socket: $!\n" unless $sock;

if ($sock) {
    print $sock "protocol management\n";
    print $sock "bundle list\n";
    my $done = 0;
    # ignore first 3 lines
    my $line = <$sock>;
    my $line = <$sock>;
    my $line = <$sock>;
        
    while (! $done) {
        my $line = <$sock>;
        if ($line =~ /dtn/) {
            print $line
        } else {
	    $done = 1;
        }
    }

    close($sock);
}
