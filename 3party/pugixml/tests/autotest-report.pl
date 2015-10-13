#!/usr/bin/perl

# pretty-printing
sub prettysuffix
{
	my $suffix = shift;

	return " C++0x" if ($suffix eq '_0x');
	return " x64" if ($suffix eq '_x64');
	return " CLR" if ($suffix eq '_clr');
	return " CLR x64" if ($suffix eq '_clr_x64');
	return " PPC" if ($suffix eq '_ppc');
	return " WinCE" if ($suffix eq '_wince');
	return " ARM" if ($suffix eq '_arm');

	return "";
}

sub prettytoolset
{
	my $toolset = shift;

	return "Borland C++ 5.82" if ($toolset eq 'bcc');
	return "Metrowerks CodeWarrior 8" if ($toolset eq 'cw');
	return "Digital Mars C++ 8.51" if ($toolset eq 'dmc');
	return "Sun C++ 5.10" . prettysuffix($1) if ($toolset =~ /^suncc(.*)$/);

	return "Intel C++ Compiler $1.0" . prettysuffix($2) if ($toolset =~ /^ic(\d+)(.*)$/);
	return "MinGW (GCC $1.$2)" . prettysuffix($3) if ($toolset =~ /^mingw(\d)(\d)(.*)$/);
	return "Microsoft Visual C++ 7.1" if ($toolset eq 'msvc71');
	return "Microsoft Visual C++ $1.0" . prettysuffix($2) if ($toolset =~ /^msvc(\d+)(.*)$/);
	return "GNU C++ Compiler $1" . prettysuffix($2) if ($toolset =~ /^gcc([\d.]*)(.*)$/);

	return "Microsoft Xbox360 Compiler" if ($toolset =~ /^xbox360/);
	return "Sony PlayStation3 GCC" if ($toolset =~ /^ps3_gcc/);
	return "Sony PlayStation3 SNC" if ($toolset =~ /^ps3_snc/);

	return "Android NDK (GCC)" . ($1 eq '_stlport' ? " STLport" : "") if ($toolset =~ /^android(.*)$/);
	return "bada SDK (GCC)" if ($toolset =~ /^bada$/);
	return "BlackBerry NDK (GCC)" if ($toolset =~ /^blackberry$/);

	$toolset;
}

sub prettyplatform
{
	my ($platform, $toolset) = @_;

	return "solaris" if ($platform =~ /solaris/);

	return "macos" if ($platform =~ /darwin/);

	return "linux64" if ($platform =~ /64-linux/);
	return "linux32" if ($platform =~ /86-linux/);

	return "fbsd64" if ($platform =~ /64-freebsd/);
	return "fbsd32" if ($platform =~ /86-freebsd/);

	return "x360" if ($toolset =~ /^xbox360/);
	return "ps3" if ($toolset =~ /^ps3/);

    return "arm" if ($toolset =~ /_arm$/);
    return "arm" if ($toolset =~ /_wince$/);
    return "arm" if ($toolset =~ /^android/);
    return "arm" if ($toolset =~ /^bada/);
    return "arm" if ($toolset =~ /^blackberry/);

	return "win64" if ($platform =~ /MSWin32-x64/);
	return "win32" if ($platform =~ /MSWin32/);

	$platform;
}

sub prettybox
{
	my $enabled = shift;
	my $color = $enabled ? "#cccccc" : "#ffffff";

	"<td bgcolor='$color' align='center'>" . ($enabled ? "+" : "&nbsp;") . "</td>";
}

# parse build log
%results = ();
%toolsets = ();
%defines = ();
%configurations = ();

sub insertindex
{
	my ($hash, $key) = @_;

	$$hash{$key} = scalar(keys %$hash) unless defined $$hash{$key};
}

while (<>)
{
	### autotest i386-freebsd-64int gcc release [wchar] result 0 97.78 98.85
	if (/^### autotest (\S+) (\S+) (\S+) \[(.*?)\] (.*)/)
	{
		my ($platform, $toolset, $configuration, $defineset, $info) = ($1, $2, $3, $4, $5);

		my $fulltool = &prettyplatform($platform, $toolset) . ' ' . &prettytoolset($toolset);
		my $fullconf = "$configuration $defineset";

		if ($info =~ /^prepare/)
		{
			$results{$fulltool}{$fullconf}{result} = "";
		}
		elsif ($info =~ /^success/)
		{
			$results{$fulltool}{$fullconf}{result} = "success";
		}
		elsif ($info =~ /^skiprun/)
		{
			$results{$fulltool}{$fullconf}{result} = "skiprun";
		}
		elsif ($info =~ /^coverage (\S+)/)
		{
			$results{$fulltool}{$fullconf}{coverage} = $1;
		}
		else
		{
			print STDERR "Unrecognized autotest infoline $_";
		}

		&insertindex(\%toolsets, $fulltool);

		$defines{$_} = 1 foreach (split /,/, $defineset);
		&insertindex(\%configurations, $fullconf);
	}
	elsif (/^### autotest revision (.+)/)
	{
		if (defined $revision && $revision != $1)
		{
			print STDERR "Autotest build report contains several revisions: $revision, $1\n";
		}
		else
		{
			$revision = $1;
		}
	}
}

# make arrays of toolsets and configurations
@toolsetarray = ();
@configurationarray = ();

$toolsetarray[$toolsets{$_}] = $_ foreach (keys %toolsets);
$configurationarray[$configurations{$_}] = $_ foreach (keys %configurations);

# print header
$stylesheet = <<END;
table.autotest { border: 1px solid black; border-left: none; border-top: none; }
table.autotest td { border: 1px solid black; border-right: none; border-bottom: none; }
END

print <<END;
<html><head><title>pugixml autotest report</title><style type="text/css"><!-- $stylesheet --></style></head><body>
<h3>pugixml autotest report</h3>
<table border=1 cellspacing=0 cellpadding=4 class="autotest">
END

# print configuration header (release/debug)
print "<tr><td align='right' colspan=2>optimization</td>";
print &prettybox((split /\s+/)[0] eq 'release') foreach (@configurationarray);
print "</tr>\n";

# print defines header (one row for each define)
foreach $define (sort {$a cmp $b} keys %defines)
{
	print "<tr><td align='right' colspan=2><small>$define</small></td>";

	foreach (@configurationarray)
	{
		my $present = ($_ =~ /\b$define\b/);

		print &prettybox($present);
	}
	print "</tr>\n";
}

# print data (one row for each toolset)
foreach $tool (@toolsetarray)
{
	my ($platform, $toolset) = split(/\s+/, $tool, 2);
	print "<tr><td style='border-right: none' align='center'><small>$platform</small></td><td style='border-left: none'><nobr>$toolset</nobr></td>";

	foreach (@configurationarray)
	{
		my $info = $results{$tool}{$_};

		if (!defined $$info{result})
		{
			print "<td bgcolor='#cccccc'>&nbsp;</td>";
		}
		elsif ($$info{result} eq "success")
		{
			my $coverage = $$info{coverage};

			print "<td bgcolor='#00ff00' align='center'>pass";

			if ($coverage > 0)
			{
				print "<br><font size='-2'>" . ($coverage + 0) . "%</font>";
			}

			print "</td>";
		}
		elsif ($$info{result} eq "skiprun")
		{
			print "<td bgcolor='#ffff80' align='center'>pass</td>"
		}
		else
		{
			print "<td bgcolor='#ff0000' align='center'>fail</td>"
		}
	}

	print "</tr>\n";
}

# print footer
$date = localtime;

print <<END;
</table><br>
Generated on $date from Git $revision
</body></html>
END
