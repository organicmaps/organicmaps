#include "grid_generator.hpp"

#include "../base/logging.hpp"

#include "../indexer/cell_id.hpp"
#include "../indexer/mercator.hpp"

// tags used for grid drawing
#define GRIDKEY "mapswithme"
#define GRIDVALUE "grid"
#define CAPTIONKEY "place"
#define CAPTIONVALUE "country"

namespace grid
{
  static size_t const MIN_GRID_LEVEL = 1;
  static size_t const MAX_GRID_LEVEL = 10;

  template <class TCellId>
  string MercatorPointToCellIdString(double x, double y, size_t bucketingLevel)
  {
    TCellId id = CellIdConverter<MercatorBounds, TCellId>::ToCellId(x, y);
    return id.ToString().substr(0, bucketingLevel);
  }

  void GenerateGridToStdout(size_t bucketingLevel)
  {
    if (bucketingLevel < MIN_GRID_LEVEL || bucketingLevel > MAX_GRID_LEVEL)
    {
      LOG(LERROR, ("Bucketing level", bucketingLevel, "for grid is not within valid range [", MIN_GRID_LEVEL, "..", MAX_GRID_LEVEL, "]"));
      return;
    }

    size_t const COUNT = 2 << (bucketingLevel - 1);
    double const STEPX = (MercatorBounds::maxX - MercatorBounds::minX) / COUNT;
    double const STEPY = (MercatorBounds::maxY - MercatorBounds::minY) / COUNT;

    cout <<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm version=\"0.6\" generator=\"MapsWithMe Indexer Tool\">\n"
    "  <bounds minlat=\"" << MercatorBounds::YToLat(MercatorBounds::minY) <<
        "\" minlon=\"" << MercatorBounds::XToLon(MercatorBounds::minX) <<
        "\" maxlat=\"" << MercatorBounds::YToLat(MercatorBounds::maxY) <<
        "\" maxlon=\"" << MercatorBounds::XToLon(MercatorBounds::maxX) << "\"/>\n";

    // generate nodes and ways
    size_t nodeID = 1;
    size_t wayID = 1;
    for (double y = MercatorBounds::minY; y <= MercatorBounds::maxY; y += STEPY)
    {
      size_t const firstID = nodeID;
      cout <<
      "  <node id=\"" << nodeID++ <<
          "\" lat=\"" << MercatorBounds::YToLat(y) <<
          "\" lon=\"" << MercatorBounds::XToLon(MercatorBounds::minX) <<
          "\"/>\n";
      size_t const secondID = nodeID;
      cout <<
      "  <node id=\"" << nodeID++ <<
          "\" lat=\"" << MercatorBounds::YToLat(y) <<
          "\" lon=\"" << MercatorBounds::XToLon(MercatorBounds::maxX) <<
          "\"/>\n";
      cout <<
      "  <way id=\"" << wayID++ << "\">\n"
      "    <nd ref=\"" << firstID << "\"/>\n"
      "    <nd ref=\"" << secondID << "\"/>\n"
      "    <tag k=\"" << GRIDKEY << "\" v=\"" << GRIDVALUE << "\"/>\n"
      "    <tag k=\"layer\" v=\"-5\"/>\n"
      "  </way>\n";
    }
    for (double x = MercatorBounds::minX; x <= MercatorBounds::maxX; x += STEPX)
    {
      size_t const firstID = nodeID;
      cout <<
      "  <node id=\"" << nodeID++ <<
          "\" lat=\"" << MercatorBounds::YToLat(MercatorBounds::minY) <<
          "\" lon=\"" << MercatorBounds::XToLon(x) <<
          "\"/>\n";
      size_t const secondID = nodeID;
      cout <<
      "  <node id=\"" << nodeID++ <<
          "\" lat=\"" << MercatorBounds::YToLat(MercatorBounds::maxY) <<
          "\" lon=\"" << MercatorBounds::XToLon(x) <<
          "\"/>\n";
      cout <<
      "  <way id=\"" << wayID++ << "\">\n"
      "    <nd ref=\"" << firstID << "\"/>\n"
      "    <nd ref=\"" << secondID << "\"/>\n"
      "    <tag k=\"" << GRIDKEY << "\" v=\"" << GRIDVALUE << "\"/>\n"
      "    <tag k=\"layer\" v=\"-5\"/>\n"
      "  </way>\n";
    }

    // generate nodes with captions
    for (size_t y = 0; y <= COUNT - 1; ++y)
    {
      for (size_t x = 0; x <= COUNT - 1; ++x)
      {
        double const mercY = MercatorBounds::minY + y * STEPY + STEPY / 2;
        double const mercX = MercatorBounds::minX + x * STEPX + STEPX / 2;
        string const title = MercatorPointToCellIdString<m2::CellId<MAX_GRID_LEVEL> >(mercX, mercY, bucketingLevel);
        cout <<
        "  <node id=\"" << nodeID++ <<
            "\" lat=\"" << MercatorBounds::YToLat(mercY) <<
            "\" lon=\"" << MercatorBounds::XToLon(mercX) <<
            "\">\n";
        cout <<
        "    <tag k=\"" << CAPTIONKEY << "\" v=\"" << CAPTIONVALUE << "\"/>\n";
        cout <<
        "    <tag k=\"name\" v=\"" << title << "\"/>\n";
        cout <<
        "  </node>\n";
      }
    }
    cout <<
    "</osm>\n";
  }

/*  void GenerateGridToStdout(size_t bucketingLevel)
  {
    if (bucketingLevel < MIN_GRID_LEVEL || bucketingLevel > MAX_GRID_LEVEL)
    {
      LOG(LERROR, ("Bucketing level", bucketingLevel, "for grid is not within valid range [", MIN_GRID_LEVEL, "..", MAX_GRID_LEVEL, "]"));
      return;
    }

    size_t const COUNT = 2 << (bucketingLevel - 1);
    double const STEPX = (MercatorBounds::maxX - MercatorBounds::minX) / COUNT;
    double const STEPY = (MercatorBounds::maxY - MercatorBounds::minY) / COUNT;

    cout <<
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm version=\"0.6\" generator=\"MapsWithMe Indexer Tool\">\n"
    "  <bounds minlat=\"" << MercatorBounds::YToLat(MercatorBounds::minY) <<
        "\" minlon=\"" << MercatorBounds::XToLon(MercatorBounds::minX) <<
        "\" maxlat=\"" << MercatorBounds::YToLat(MercatorBounds::maxY) <<
        "\" maxlon=\"" << MercatorBounds::XToLon(MercatorBounds::maxX) << "\"/>\n";

    // generate nodes
    size_t nodeID = 1;
    for (double y = MercatorBounds::minY; y <= MercatorBounds::maxY; y += STEPY)
    {
      for (double x = MercatorBounds::minX; x <= MercatorBounds::maxX; x += STEPX)
      {
        cout <<
        "  <node id=\"" << nodeID++ <<
            "\" lat=\"" << MercatorBounds::YToLat(y) <<
            "\" lon=\"" << MercatorBounds::XToLon(x) <<
            "\"/>\n";
      }
    }
    // generate squares and captions
    size_t wayID = 1;
    for (size_t y = 0; y <= COUNT - 1; ++y)
    {
      for (size_t x = 0; x <= COUNT - 1; ++x)
      {
        size_t const first = x + y * (COUNT + 1) + 1;
        size_t const second = first + 1;
        size_t const third = second + COUNT + 1;
        size_t const fourth = third - 1;
        string title = CellStringFromXYLevel(x, y, bucketingLevel);
        ++nodeID;
        cout <<
        "  <node id=\"" << nodeID <<
            "\" lat=\"" << MercatorBounds::YToLat(MercatorBounds::minY + y * STEPY + STEPY / 2) <<
            "\" lon=\"" << MercatorBounds::XToLon(MercatorBounds::minX + x * STEPX + STEPX / 2) <<
            "\">\n";
        cout <<
        "    <tag k=\"" << TAGKEY << "\" v=\"" << CAPTIONVALUE << "\"/>\n";
        cout <<
        "    <tag k=\"name\" v=\"" << title << "\"/>\n";
        cout <<
        "  </node>\n";

        cout <<
        "  <way id=\"" << wayID++  << "\">\n"
        "    <nd ref=\"" << first   << "\"/>\n"
        "    <nd ref=\"" << second << "\"/>\n"
        "    <nd ref=\"" << third  << "\"/>\n"
        "    <nd ref=\"" << fourth << "\"/>\n"
        "    <nd ref=\"" << first   << "\"/>\n"
        "    <tag k=\"name\" v=\"" << title << "\"/>\n"
        "    <tag k=\"" << TAGKEY << "\" v=\"" << GRIDVALUE << "\"/>\n"
        "    <tag k=\"layer\" v=\"-5\"/>\n"
        "  </way>\n";
      }
    }
    cout <<
    "</osm>\n";
  }*/
}
