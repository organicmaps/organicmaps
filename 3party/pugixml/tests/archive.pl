#!/usr/bin/perl

use Archive::Tar;
use Archive::Zip;
use File::Basename;

my $target = shift @ARGV;
my @sources = @ARGV;

my $basedir = basename($target, ('.zip', '.tar.gz', '.tgz')) . '/';

my $zip = $target =~ /\.zip$/;
my $arch = $zip ? Archive::Zip->new : Archive::Tar->new;

for $source (sort {$a cmp $b} @sources)
{
	my $contents = &readfile_contents($source);
	my $meta = &readfile_meta($source);
	my $file = $basedir . $source;

    if (-T $source)
    {
        # convert all newlines to Unix format
        $contents =~ s/\r//g;

        if ($zip)
        {
            # convert all newlines to Windows format for .zip distribution
            $contents =~ s/\n/\r\n/g;
        }
    }

	if ($zip)
	{
		my $path = $file;
		$arch->addDirectory($path) if $path =~ s/\/[^\/]+$/\// && !defined($arch->memberNamed($path));

		my $member = $arch->addString($contents, $file);

		$member->desiredCompressionMethod(COMPRESSION_DEFLATED);
		$member->desiredCompressionLevel(9);

		$member->setLastModFileDateTimeFromUnix($$meta{mtime});
	}
	else
	{
		$arch->add_data($file, $contents, $meta);
	}
}

$zip ? $arch->overwriteAs($target) : $arch->write($target, 9);

sub readfile_contents
{
	my $file = shift;

	open FILE, $file or die "Can't open $file: $!";
	binmode FILE;
	my @contents = <FILE>;
	close FILE;

	return join('', @contents);
}

sub readfile_meta
{
	my $file = shift;

    my ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks) = stat($file);

	return {mtime => $mtime};
}
