#!/usr/bin/perl

sub funcinfo
{
	my ($name, $info) = @_;

	return if ($info =~ /No executable lines/);

	my $lines = ($info =~ /Lines executed:([^%]+)%/) ? $1 : 100;
	my $calls = ($info =~ /Calls executed:([^%]+)%/) ? $1 : 100;
	my $branches = ($info =~ /Branches executed:([^%]+)%/) ? $1 : 100;
	my $taken = ($info =~ /Taken at least once:([^%]+)%/) ? $1 : 100;

	return if ($lines == 100 && $calls == 100 && $branches == 100 && $taken == 100);

	return "Function $name: L $lines, C $calls, B $branches, BT $taken\n";
}

$prefix = join(' ', @ARGV);
$prefix .= ' ' if ($prefix ne '');

$lines = join('', <STDIN>);

# merge file information
$lines =~ s/File (.+)\nLines (.+)\n(.+\n)*\n/$1 $2\n/g;

# merge function information
$lines =~ s/Function (.+)\n((.+\n)*)\n/funcinfo($1, $2)/eg;

# remove include information
$lines =~ s/.+include\/c\+\+.+\n//g;

foreach $line (split /\n/, $lines)
{
	print "$prefix$line\n";
}
