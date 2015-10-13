#!/usr/bin/perl

use IO::Socket;

$vm = shift;
$log = shift;

# start virtualbox gui in minimized mode - this should be the first thing we do since this process
# inherits all handles and we want our sockets/log file closed
system("start /min virtualbox --startvm $vm");

# start a server; vm will connect to the server via autotest-remote-host.pl
my $server = new IO::Socket::INET(LocalPort => 7183, Listen => 1);
die "Could not create socket: $!\n" unless $server;

open LOG, ">> $log" || die "Could not open log file: $!\n";

print LOG "Listening for connection...\n";

my $client = $server->accept();

# echo all input to log file
print LOG $_ while (<$client>);
close LOG;

$client->close();
$server->close();

# wait for vm shutdown to decrease peak memory consumption
while (`vboxmanage showvminfo $vm` !~ /State:\s+powered off/)
{
    sleep(1);
}
