#include "generator/generator/kml_parser.hpp"
#include "generator/generator/feature_sorter.hpp"

#include "generator/base/string_utils.hpp"
#include "generator/base/logging.hpp"

#include "generator/coding/parse_xml.hpp"
#include "generator/coding/file_reader.hpp"

#include "generator/geometry/rect2d.hpp"
#include "generator/geometry/cellid.hpp"

#include "generator/indexer/cell_id.hpp"
#include "generator/geometry/mercator.hpp"
#include "generator/indexer/feature.hpp"
#include "generator/indexer/covering.hpp"

#include "generator/std/fstream.hpp"

#define MIN_SIMPLIFIED_POINTS_COUNT 4

namespace feature
{
  typedef vector<m2::PointD> points_t;
  void TesselateInterior(points_t const & bound, list<points_t> const & holes,
                        points_t & triangles);
}

namespace kml
{
  typedef vector<Region> PolygonsContainerT;

  class KmlParser
  {
    vector<std::string> m_tags;
    /// buffer for text with points
    std::string m_data;
    std::string m_name;

    PolygonsContainerT & m_country;
    int m_level;

  public:
    KmlParser(PolygonsContainerT & country, int level);

    bool Push(std::string const & element);
    void Pop(std::string const & element);
    void AddAttr(std::string const &, std::string const &) {}
    void CharData(std::string const & data);
  };

  KmlParser::KmlParser(PolygonsContainerT & country, int level)
    : m_country(country), m_level(level)
  {
  }

  bool KmlParser::Push(std::string const & element)
  {
    m_tags.push_back(element);

    return true;
  }

  template <class PointsContainerT>
  class PointsCollector
  {
    PointsContainerT & m_container;

  public:
    PointsCollector(PointsContainerT & container) : m_container(container)
    {
    }

    void operator()(std::string const & latLon)
    {
      size_t const firstCommaPos = latLon.find(',');
      CHECK(firstCommaPos != std::string::npos, ("invalid latlon", latLon));
      std::string const lonStr = latLon.substr(0, firstCommaPos);
      double lon;
      CHECK(utils::to_double(lonStr, lon), ("invalid lon", lonStr));
      size_t const secondCommaPos = latLon.find(',', firstCommaPos + 1);
      std::string latStr;
      if (secondCommaPos == std::string::npos)
        latStr = latLon.substr(firstCommaPos + 1);
      else
        latStr = latLon.substr(firstCommaPos + 1, secondCommaPos - firstCommaPos - 1);
      double lat;
      CHECK(utils::to_double(latStr, lat), ("invalid lon", latStr));
      // to mercator
      m2::PointD const mercPoint(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
      m_container.push_back(mercPoint);
    }
  };

  class AreaFeature : public FeatureType
  {
  public:
    template <class IterT>
    AreaFeature(IterT beg, IterT end)
    {
      // manually fill bordering geometry points
      m_bPointsParsed = true;
      for (IterT it = beg; it != end; ++it)
      {
        m_Points.push_back(*it);
        m_LimitRect.Add(*it);
      }

      // manually fill triangles points
      m_bTrianglesParsed = true;
      list<feature::points_t> const holes;
      feature::points_t points(beg, end);
      feature::points_t triangles;
      feature::TesselateInterior(points, holes, triangles);
      CHECK(!triangles.empty(), ("Tesselation unsuccessfull?"));
      for (size_t i = 0; i < triangles.size(); ++i)
        m_Triangles.push_back(triangles[i]);
    }
  };
  void KmlParser::Pop(std::string const & element)
  {
    if (element == "Placemark")
    {
    }
    else if (element == "coordinates")
    {
      size_t const size = m_tags.size();
      CHECK(m_tags.size() > 3, ());
      CHECK(m_tags[size - 2] == "LinearRing", ());

      if (m_tags[size - 3] == "outerBoundaryIs")
      {
        // first, collect points in Mercator
        typedef vector<m2::PointD> MercPointsContainerT;
        MercPointsContainerT points;
        PointsCollector<MercPointsContainerT> collector(points);
        utils::TokenizeString(m_data, " \n\r\a", collector);
        size_t const numPoints = points.size();
        if (numPoints > 3 && points[numPoints - 1] == points[0])
        {
//          // create feature for country's polygon
//          AreaFeature ft(points.begin(), points.end());
//          // get polygon covering (cellids)
//          vector<int64_t> ids;
//          ids = covering::CoverFeature(ft, -1);
//          // debug output
//          set<int64_t> ids8;
//          for (size_t i = 0; i < ids.size(); ++i)
//          {
//            int64_t a = ids[i] >> (2 * 11);
//            if (ids8.insert(a).second)
//              LOG(LINFO, (RectId::FromInt64(a).ToString()));
//          }
//          LOG(LINFO, ("Total cellids:", ids8.size()));
          // second, simplify points if necessary
          if (m_level > 0)
          {
            MercPointsContainerT simplifiedPoints;
            feature::SimplifyPoints(points, simplifiedPoints, m_level);
            if (simplifiedPoints.size() > MIN_SIMPLIFIED_POINTS_COUNT)
            {
              // LOG_SHORT(LINFO, (m_name, numPoints, "simplified to ", simplifiedPoints.size()));
              points.swap(simplifiedPoints);
            }
            else
            {
              // LOG_SHORT(LINFO, (m_name, numPoints, "NOT simplified"));
            }
          }

          // remove last point which is equal to first
          // it's not needed for Region::Contains
          points.pop_back();

          m_country.push_back(Region());
          for (MercPointsContainerT::iterator it = points.begin(); it != points.end(); ++it)
            m_country.back().AddPoint(*it);
        }
        else
        {
          LOG(LWARNING, ("Invalid region for country"/*, m_country.m_name*/));
        }
      }
      else if (m_tags[size - 3] == "innerBoundaryIs")
      { // currently we're ignoring holes
      }
      else
      {
        CHECK(false, ("Unsupported tag", m_tags[size - 3]));
      }

      m_data.clear();
    }
    else if (element == "Polygon")
    {
    }

    m_tags.pop_back();
  }

  void KmlParser::CharData(std::string const & data)
  {
    size_t const size = m_tags.size();

    if (size > 1 && m_tags[size - 1] == "name" && m_tags[size - 2] == "Placemark")
    {
      m_name = data;
    }
    else if (size > 4 && m_tags[size - 1] == "coordinates"
        && m_tags[size - 2] == "LinearRing" && m_tags[size - 4] == "Polygon")
    {
      // text block can be really huge
      m_data.append(data);
    }
  }

  bool LoadPolygons(std::string const & kmlFile, PolygonsContainerT & country, int level)
  {
    KmlParser parser(country, level);
    try
    {
      FileReader file(kmlFile);
      ReaderSource<FileReader> source(file);
      bool const bRes = ParseXML(source, parser, true);
      return bRes;
    }
    catch (std::exception const &)
    {
    }
    return false;
  }
}
