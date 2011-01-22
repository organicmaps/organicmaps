#include "kml_parser.hpp"
#include "feature_sorter.hpp"

#include "../../base/string_utils.hpp"
#include "../../base/logging.hpp"

#include "../../coding/parse_xml.hpp"
#include "../../coding/file_reader.hpp"

#include "../../geometry/rect2d.hpp"

#include "../../indexer/cell_id.hpp"
#include "../../indexer/mercator.hpp"

#include "../../std/fstream.hpp"

#define POLYGONS_FILE "polygons.lst"
#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".kml"

#define MIN_SIMPLIFIED_POINTS_COUNT 10

namespace kml
{

  class KmlParser
  {
    vector<string> m_tags;
    /// buffer for text with points
    string m_data;

    CountryPolygons & m_country;
    int m_simplifyCountriesLevel;

  public:
    KmlParser(CountryPolygons & country, int simplifyCountriesLevel);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const &, string const &) {}
    void CharData(string const & data);
  };

  KmlParser::KmlParser(CountryPolygons & country, int simplifyCountriesLevel)
    : m_country(country), m_simplifyCountriesLevel(simplifyCountriesLevel)
  {
  }

  bool KmlParser::Push(string const & element)
  {
    m_tags.push_back(element);

    return true;
  }

  template <class PointsContainerT>
  class PointsCollector
  {
    PointsContainerT & m_container;
    m2::RectD & m_rect;

  public:
    PointsCollector(PointsContainerT & container, m2::RectD & rect)
      : m_container(container), m_rect(rect) {}

    void operator()(string const & latLon)
    {
      size_t const firstCommaPos = latLon.find(',');
      CHECK(firstCommaPos != string::npos, ("invalid latlon", latLon));
      string const lonStr = latLon.substr(0, firstCommaPos);
      double lon;
      CHECK(utils::to_double(lonStr, lon), ("invalid lon", lonStr));
      size_t const secondCommaPos = latLon.find(',', firstCommaPos + 1);
      string latStr;
      if (secondCommaPos == string::npos)
        latStr = latLon.substr(firstCommaPos + 1);
      else
        latStr = latLon.substr(firstCommaPos + 1, secondCommaPos - firstCommaPos - 1);
      double lat;
      CHECK(utils::to_double(latStr, lat), ("invalid lon", latStr));
      // to mercator
      m2::PointD const mercPoint(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
      m_rect.Add(mercPoint);
      m_container.push_back(mercPoint);
    }
  };

  m2::PointU MercatorPointToPointU(m2::PointD const & pt)
  {
    typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;
    uint32_t const ix = static_cast<uint32_t>(CellIdConverterType::XToCellIdX(pt.x));
    uint32_t const iy = static_cast<uint32_t>(CellIdConverterType::YToCellIdY(pt.y));
    return m2::PointU(ix, iy);
  }

  void KmlParser::Pop(string const & element)
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
        m2::RectD rect;
        PointsCollector<MercPointsContainerT> collector(points, rect);
        utils::TokenizeString(m_data, " \n\r\a", collector);
        size_t const numPoints = points.size();
        if (numPoints > 3 && points[numPoints - 1] == points[0])
        {
          // remove last point which is equal to first
          points.pop_back();

          // second, simplify points if necessary
          if (m_simplifyCountriesLevel > 0)
          {
            MercPointsContainerT simplifiedPoints;
            feature::SimplifyPoints(points, simplifiedPoints, m_simplifyCountriesLevel);
            if (simplifiedPoints.size() > MIN_SIMPLIFIED_POINTS_COUNT)
            {
              LOG_SHORT(LINFO, (m_country.m_name, numPoints, "simplified to ", simplifiedPoints.size()));
              points.swap(simplifiedPoints);
            }
          }

          // third, convert mercator doubles to uint points
          Region region;
          for (MercPointsContainerT::iterator it = points.begin(); it != points.end(); ++it)
            region.AddPoint(MercatorPointToPointU(*it));

          m_country.m_regions.push_back(region);
          m_country.m_rect.Add(rect);
        }
        else
        {
          LOG(LWARNING, ("Invalid region for country", m_country.m_name));
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

  void KmlParser::CharData(string const & data)
  {
    size_t const size = m_tags.size();

    if (size > 1 && m_tags[size - 1] == "name" && m_tags[size - 2] == "Placemark")
      m_country.m_name = data;
    else if (size > 4 && m_tags[size - 1] == "coordinates"
        && m_tags[size - 2] == "LinearRing" && m_tags[size - 4] == "Polygon")
    {
      // text block can be really huge
      m_data.append(data);
    }
  }

  bool LoadPolygonsFromKml(string const & kmlFile, CountryPolygons & country,
                           int simplifyCountriesLevel)
  {
    KmlParser parser(country, simplifyCountriesLevel);
    try
    {
      FileReader file(kmlFile);
      ReaderSource<FileReader> source(file);
      return ParseXML(source, parser, true);
    }
    catch (std::exception const &)
    {
    }
    return false;
  }

  class PolygonLoader
  {
    string m_baseDir;
    CountryPolygons & m_out;
    int m_simplifyCountriesLevel;

  public:
    PolygonLoader(string const & basePolygonsDir, CountryPolygons & polygons,
                  int simplifyCountriesLevel)
      : m_baseDir(basePolygonsDir), m_out(polygons),
        m_simplifyCountriesLevel(simplifyCountriesLevel) {}
    void operator()(string const & name)
    {
      if (m_out.m_name.empty())
        m_out.m_name = name;
      CountryPolygons current;
      if (LoadPolygonsFromKml(m_baseDir + BORDERS_DIR + name + BORDERS_EXTENSION, current, m_simplifyCountriesLevel)
          && current.m_regions.size())
      {
        m_out.m_regions.insert(m_out.m_regions.end(),
                          current.m_regions.begin(), current.m_regions.end());
        m_out.m_rect.Add(current.m_rect);
      }
    }
  };

  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries,
                         int simplifyCountriesLevel)
  {
    if (simplifyCountriesLevel > 0)
    {
      LOG_SHORT(LINFO, ("Simplificator level for country polygons:", simplifyCountriesLevel));
    }

    countries.clear();
    ifstream stream((baseDir + POLYGONS_FILE).c_str());
    std::string line;

    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;
      CountryPolygons country;
      PolygonLoader loader(baseDir, country, simplifyCountriesLevel);
      utils::TokenizeString(line, "|", loader);
      if (!country.m_regions.empty())
        countries.push_back(country);
    }
    return !countries.empty();
  }
}
