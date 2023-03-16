#!/usr/bin/perl -w

# modules
use strict;

# tags used for grid drawing
my $TAGKEY="mapswithme";
my $GRIDVALUE="grid";
my $CAPTIONVALUE="gridcaption";

sub UsageAndExit
{
  print("Generate OSM xml with world grid\n");
  print("Usage: osm_grid_create.pl <bucketing_level>\n");
  print("  where bucketing_level is [1..10]\n");
  exit;
}

sub CellStringFromXYLevel
{
  my $x = $_[0];
  my $y = $_[1];
  my $level = $_[2];
  my $result = "";
  while ($level > 0) {
    if ($x % 2) {
      if ($y % 2) {
        $result .= "0";        
      } else {
        $result .= "3";
      }
    } else {
      if ($y % 2) {
        $result .= "1";
      } else {
        $result .= "2";
      }
    }
    $level--;
    $x /= 2;
    $y /= 2;
  }
  $result = reverse($result);
}

#######################################
# ENTRY POINT
#######################################

&UsageAndExit() if ($#ARGV < 0);
my $LEVEL = $ARGV[0];
&UsageAndExit() if ($LEVEL > 10 || $LEVEL < 1);

my $COUNT = 2 ** $LEVEL;
my $MINX = -180.0;
my $MAXX = 180.0;
my $MINY = -90.0;
my $MAXY = 90.0;
my $STEPX = ($MAXX - $MINX)/$COUNT;
my $STEPY = ($MAXY - $MINY)/$COUNT;

print <<OSM_HEADER;
<?xml version="1.0" encoding="UTF-8"?>
<osm version="0.6" generator="MapsWithMe">
  <bounds minlat="$MINY" minlon="$MINX" maxlat="$MAXY" maxlon="$MAXX"/>
OSM_HEADER

# generate nodes
my $nodeID = 1;
for (my $y = $MINY; $y <= $MAXY; $y += $STEPY)
{
  for (my $x = $MINX; $x <= $MAXX; $x += $STEPX)
  {
    printf("  <node id=\"$nodeID\" lat=\"%.10f\" lon=\"%.10f\"/>\n", $y, $x);
    $nodeID++;
  }
}
# generate squares and captions
my $wayID = 10000;
foreach my $y (0..$COUNT - 1)
{
  foreach my $x (0..$COUNT - 1)
  {
    my $first = $x + $y * ($COUNT + 1) + 1;
    my $second = $first + 1;
    my $third = $second + $COUNT + 1;
    my $fourth = $third - 1;
    my $title = &CellStringFromXYLevel($x, $y, $LEVEL);
    $nodeID++;
    printf("  <node id=\"$nodeID\" lat=\"%.10f\" lon=\"%.10f\">\n", $MINY + $y * $STEPY + $STEPY/2, $MINX + $x * $STEPX + $STEPX/2);
    printf("    <tag k=\"$TAGKEY\" v=\"$CAPTIONVALUE\"/>\n");
    printf("    <tag k=\"name\" v=\"$title\"/>\n");
    printf("  </node>\n");
    
    print <<OSM_GRID_TILE;
  <way id="$wayID">
    <nd ref="$first"/>
    <nd ref="$second"/>
    <nd ref="$third"/>
    <nd ref="$fourth"/>
    <nd ref="$first"/>
    <tag k="name" v="$title"/>
    <tag k="$TAGKEY" v="$GRIDVALUE"/>
    <tag k="layer" v="-5"/>
  </way>
OSM_GRID_TILE
    $wayID++;
  }
}

print <<OSM_FOOTER;
</osm>
OSM_FOOTER
