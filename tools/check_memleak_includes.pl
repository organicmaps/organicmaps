#!/usr/bin/perl -w

# modules
use strict;
use File::Find;

my $ROOT = "..";
my @EXCLUDE = ("3party", "base", "std", "out", "tools", "testing");
my @SOURCE_EXT = (".cpp");
my @HEADER_EXT = (".hpp", ".h");

my $START_MD = "start_mem_debug.hpp";
my $STOP_MD = "stop_mem_debug.hpp";
my $INCLUDE_KEYWORD = "#include";

#######################
sub analyze_header {
  # search for start inlude, in header file after it should be no more includes!
  # and stop include should be the last at the end
  my $res = index($_[1], $START_MD);
  if ($res == -1) {
    print "ERROR: No memory leak detector in file $_[0]\n";
  }
  else {
    my $res2 = index($_[1], $STOP_MD, $res);
    if ($res2 == -1) {
      print "ERROR: Last line in header file $_[0] should contain #include \"$STOP_MD\"\n";
    }
    else
    {
      print "$_[0] is OK\n";
    }
  }
}

###################
sub analyze_source {
  # search for start inlude, in source file it should be only one and after it no more includes!
  my $res = index($_[1], $START_MD);
  if ($res == -1) {
    print "ERROR: No memory leak detector in file $_[0]\n";
  }
  else {
    my $res2 = index($_[1], $INCLUDE_KEYWORD, $res);
    if ($res2 != -1) {
      print "ERROR: #include \"$START_MD\" should be the last include in $_[0]\n";
    }
    else
    {
      print "$_[0] is OK\n";
    }
  }
}

###################################################################
sub process_file {
  my $fullPath = $File::Find::name;
  my $f = $_;

  # ignore tests directories
  unless ($fullPath =~ /_tests\//)
  {
    my $isSource = 0;
    foreach my $ext (@SOURCE_EXT) {
      $isSource = 1 if ($f =~ /$ext$/);
    }
    my $isHeader = 0;
    foreach my $ext (@HEADER_EXT) {
      $isHeader = 1 if ($f =~ /$ext$/);
    }

    if ($isSource or $isHeader) {
      open(INFILE, "<$f") or die("ERROR: can't open input file $fullPath\n");
      binmode(INFILE);
      my @fileAttribs = stat(INFILE);
      read(INFILE, my $buffer, $fileAttribs[7]);
      close(INFILE);
      analyze_source($fullPath, $buffer) if $isSource;
      analyze_header($fullPath, $buffer) if $isHeader;
    }
  }
}


#######################################
# ENTY POINT
#######################################

print("Scanning sources for correct memory leak detector includes\n");
my @raw_dirs = <$ROOT/*>;
my @dirs;

# filter out excluded directories
foreach my $f (@raw_dirs) {
  my $good = 1;
  foreach my $excl (@EXCLUDE) {
    if (-f $f or $f =~ /$excl/) {
      $good = 0;
    }
  }
  push(@dirs, $f) if $good;
}

# set array print delimeter
print "Directories for checking:\n@dirs\n";
find(\&process_file, @dirs);
