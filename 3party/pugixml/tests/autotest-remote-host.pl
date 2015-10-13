#!/usr/bin/perl

sub execprint
{
	my $cmd = shift;

	open PIPE, "$cmd |" || die "$cmd failed: $!\n";
	print while (<PIPE>);
	close PIPE;

	return $?;
}

use IO::Socket;
use Net::Ping;

$exitcmd = shift;
$host = "10.0.2.2";

# wait while network is up
$ping = Net::Ping->new("icmp");

while (!$ping->ping($host))
{
	print "### autotest $host is down, retrying...\n";
}

print "### autotest $host is up, connecting...\n";

my $client = new IO::Socket::INET(PeerAddr => "$host:7183");
exit unless $client;

select $client;

&execprint('git pull') == 0 || die "error updating from repo\n";
&execprint('perl tests/autotest-local.pl') == 0 || die "error launching tests\n";
system($exitcmd);
