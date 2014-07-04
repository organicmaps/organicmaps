#include "borders_generator.hpp"
#include "osm_xml_parser.hpp"

#include "../base/std_serialization.hpp"
#include "../base/logging.hpp"

#include "../indexer/mercator.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/streams_sink.hpp"
#include "../coding/parse_xml.hpp"

#include "../std/list.hpp"
#include "../std/vector.hpp"

namespace osm
{
  class BordersCreator
  {
    vector<m2::RegionD> & m_borders;
    OsmRawData const & m_data;
    m2::RegionD m_currentRegion;

  public:
    BordersCreator(vector<m2::RegionD> & outBorders, OsmRawData const & data)
      : m_borders(outBorders), m_data(data)
    {
    }

    void CreateFromWays(OsmWays const & ways)
    {
      for (OsmWays::const_iterator it = ways.begin(); it != ways.end(); ++it)
      {
        // clear region
        m_currentRegion = m2::RegionD();
        it->ForEachPoint(*this);
        CHECK(m_currentRegion.IsValid(), ("Invalid region"));
        m_borders.push_back(m_currentRegion);
      }
    }

    void operator()(OsmId nodeId)
    {
      try
      {
        OsmNode node = m_data.NodeById(nodeId);
        m_currentRegion.AddPoint(MercatorBounds::FromLatLon(node.m_lat, node.m_lon));
      }
      catch (OsmRawData::OsmInvalidIdException const & e)
      {
        LOG(LWARNING, (e.what()));
      }
    }
  };

  void GenerateBordersFromOsm(string const & tagAndOptValue,
                              string const & osmFile,
                              string const & outFile)
  {
    OsmRawData osmData;
    {
      FileReader file(osmFile);
      ReaderSource<FileReader> source(file);

      OsmXmlParser parser(osmData);
      CHECK(ParseXML(source, parser), ("Invalid XML"));
    }

    // extract search tag key and value
    size_t const delimeterPos = tagAndOptValue.find('=');
    string const searchKey = tagAndOptValue.substr(0, delimeterPos);
    string searchValue;
    if (delimeterPos != string::npos)
      searchValue = tagAndOptValue.substr(delimeterPos + 1, string::npos);

    // find country borders relation
    OsmIds relationIds;
    if (searchValue.empty())
      relationIds = osmData.RelationsByKey(searchKey);
    else
      relationIds = osmData.RelationsByTag(OsmTag(searchKey, searchValue));

    CHECK(!relationIds.empty(), ("No relation found with tag", searchKey, searchValue));
    CHECK_EQUAL(relationIds.size(), 1, ("Found more than one relation with tag",
                                        searchKey, searchValue));

    OsmRelation countryRelation = osmData.RelationById(relationIds[0]);

    // get country name
    string countryName;
    if (!countryRelation.TagValueByKey("name:en", countryName))
      countryRelation.TagValueByKey("name", countryName);
    LOG(LINFO, ("Processing boundaries for country", countryName));

    // find border ways
    OsmIds wayIdsOuterRole = countryRelation.MembersByTypeAndRole("way", "outer");
    OsmIds wayIdsNoRole = countryRelation.MembersByTypeAndRole("way", "");
    // collect all ways
    list<OsmWay> ways;
    for(size_t i = 0; i < wayIdsOuterRole.size(); ++i)
      ways.push_back(osmData.WayById(wayIdsOuterRole[i]));
    for(size_t i = 0; i < wayIdsNoRole.size(); ++i)
      ways.push_back(osmData.WayById(wayIdsNoRole[i]));
    CHECK(!ways.empty(), ("No any ways in country border"));

    // merge all collected ways
    OsmWays mergedWays;
    do
    {
      OsmId lastMergedWayId = -1; // for debugging
      OsmWay merged = ways.front();
      size_t lastSize = ways.size();
      ways.pop_front();
      // repeat until we merge everything
      while (lastSize > ways.size())
      {
        lastSize = ways.size();
        for (list<OsmWay>::iterator it = ways.begin(); it != ways.end(); ++it)
        {
          if (merged.MergeWith(*it))
          {
            lastMergedWayId = it->Id();
            ways.erase(it);
            break;
          }
        }
      }
      if (!merged.IsClosed())
      {
        LOG(LERROR, ("Way merge unsuccessful as borders are not closed. Last merged id:",
                     lastMergedWayId == -1 ? merged.Id() : lastMergedWayId));
      }
      else
      {
        LOG(LINFO, ("Successfully merged boundaries with points count:", merged.PointsCount()));
        mergedWays.push_back(merged);
      }
    } while (!ways.empty());

    CHECK(!mergedWays.empty(), ("No borders were generated for country:", countryName));

    // save generated borders
    vector<m2::RegionD> borders;
    BordersCreator doCreateBorders(borders, osmData);
    doCreateBorders.CreateFromWays(mergedWays);
    CHECK_EQUAL(mergedWays.size(), borders.size(), ("Can't generate borders from ways"));

    FileWriter writer(outFile);
    stream::SinkWriterStream<Writer> stream(writer);
    stream << borders;

    LOG(LINFO, ("Saved", borders.size(), "border(s) for", countryName));
  }

  bool LoadBorders(string const & borderFile, vector<m2::RegionD> & outBorders)
  {
    try
    {
      FileReader file(borderFile);
      ReaderSource<FileReader> source(file);
      stream::SinkReaderStream<ReaderSource<FileReader> > stream(source);

      stream >> outBorders;
      CHECK(!outBorders.empty(), ("No borders were loaded from", borderFile));
    }
    catch (FileReader::OpenException const &)
    {
      return false;
    }
    return true;
  }
}
