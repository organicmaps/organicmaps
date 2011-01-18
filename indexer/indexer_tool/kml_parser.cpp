#include "kml_parser.hpp"

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

namespace kml
{

  class KmlParser
  {
    vector<string> m_tags;
    /// buffer for text with points
    string m_data;

    CountryPolygons & m_country;

  public:
    KmlParser(CountryPolygons & country);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const &, string const &) {}
    void CharData(string const & data);
  };

  KmlParser::KmlParser(CountryPolygons & country) : m_country(country)
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
      double const x = MercatorBounds::LonToX(lon);
      double const y = MercatorBounds::LatToY(lat);
      m_rect.Add(m2::PointD(x, y));
      // convert points to uint32_t
      typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;
      uint32_t const ix = static_cast<uint32_t>(CellIdConverterType::XToCellIdX(x));
      uint32_t const iy = static_cast<uint32_t>(CellIdConverterType::YToCellIdY(y));
      m_container.push_back(Region::value_type(ix, iy));
    }
  };

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
        typedef vector<Region::value_type> ContainerT;
        ContainerT points;
        m2::RectD rect;
        PointsCollector<ContainerT> collector(points, rect);
        utils::TokenizeString(m_data, " \n\r\a", collector);
        size_t const numPoints = points.size();
        if (numPoints > 3 && points[numPoints - 1] == points[0])
        {
          // remove last point which is equal to first
          points.pop_back();
          m_country.m_regions.push_back(Region(points.begin(), points.end()));
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

  bool LoadPolygonsFromKml(string const & kmlFile, CountryPolygons & country)
  {
    KmlParser parser(country);
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

  public:
    PolygonLoader(string const & basePolygonsDir, CountryPolygons & polygons)
      : m_baseDir(basePolygonsDir), m_out(polygons) {}
    void operator()(string const & name)
    {
      if (m_out.m_name.empty())
        m_out.m_name = name;
      CountryPolygons current;
      if (LoadPolygonsFromKml(m_baseDir + BORDERS_DIR + name + BORDERS_EXTENSION, current)
          && current.m_regions.size())
      {
        m_out.m_regions.insert(m_out.m_regions.end(),
                          current.m_regions.begin(), current.m_regions.end());
        m_out.m_rect.Add(current.m_rect);
      }
    }
  };

  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries)
  {
    countries.clear();
    ifstream stream((baseDir + POLYGONS_FILE).c_str());
    std::string line;

    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;
      CountryPolygons country;
      PolygonLoader loader(baseDir, country);
      utils::TokenizeString(line, ",", loader);
      if (!country.m_regions.empty())
        countries.push_back(country);
    }
    return !countries.empty();
  }
}
