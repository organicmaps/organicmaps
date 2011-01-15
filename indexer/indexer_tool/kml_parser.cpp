#include "kml_parser.hpp"

#include "../../base/string_utils.hpp"

#include "../../coding/parse_xml.hpp"
#include "../../coding/file_reader.hpp"

#include "../../indexer/cell_id.hpp"
#include "../../indexer/mercator.hpp"

#include "../../storage/simple_tree.hpp"

#include "../../std/iostream.hpp"
#include "../../std/vector.hpp"
#include "../../std/fstream.hpp"

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

  typedef boost::polygon::point_data<TCoordType> Point;
  typedef vector<Point> PointsContainer;

  class PointsCollector
  {
    PointsContainer & m_points;
    m2::RectD & m_rect;

  public:
    PointsCollector(PointsContainer & container, m2::RectD & rect)
      : m_points(container), m_rect(rect) {}

    void operator()(string const & latLon)
    {
      size_t const firstCommaPos = latLon.find(',');
      CHECK(firstCommaPos != string::npos, ("invalid latlon", latLon));
      string const latStr = latLon.substr(0, firstCommaPos);
      double lat;
      CHECK(utils::to_double(latStr, lat), ("invalid lat", latStr));
      size_t const secondCommaPos = latLon.find(',', firstCommaPos + 1);
      string lonStr;
      if (secondCommaPos == string::npos)
        lonStr = latLon.substr(firstCommaPos + 1);
      else
        lonStr = latLon.substr(firstCommaPos + 1, secondCommaPos - firstCommaPos - 1);
      double lon;
      CHECK(utils::to_double(lonStr, lon), ("invalid lon", lonStr));
      // to mercator
      double const x = MercatorBounds::LonToX(lon);
      double const y = MercatorBounds::LatToY(lat);
      m_rect.Add(m2::PointD(x, y));
      // convert points to uint32_t
      typedef CellIdConverter<MercatorBounds, RectId> CellIdConverterType;
      uint32_t const ix = static_cast<uint32_t>(CellIdConverterType::XToCellIdX(x));
      uint32_t const iy = static_cast<uint32_t>(CellIdConverterType::YToCellIdY(y));
      m_points.push_back(Point(ix, iy));
    }
  };

  void KmlParser::Pop(string const & element)
  {
    if (element == "Placemark")
    {
    }
    else if (element == "coordinates")
    {
      PointsContainer points;
      PointsCollector collector(points, m_country.m_rect);
      utils::TokenizeString(m_data, " \n\r\a", collector);
      CHECK(!points.empty(), ());

      size_t const size = m_tags.size();
      CHECK(m_tags.size() > 3, ());
      CHECK(m_tags[size - 2] == "LinearRing", ());

      using namespace boost::polygon::operators;

      if (m_tags[size - 3] == "outerBoundaryIs")
      {
        Polygon polygon;
        set_points(polygon, points.begin(), points.end());
        m_country.m_polygons.push_back(polygon);
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

  void LoadPolygonsFromKml(string const & kmlFile, CountryPolygons & country)
  {
    KmlParser parser(country);
    {
      FileReader file(kmlFile);
      ReaderSource<FileReader> source(file);
      CHECK(ParseXML(source, parser, true), ("Error while parsing", kmlFile));
    }
  }

  typedef SimpleTree<CountryPolygons> TCountriesTree;
  bool LoadCountriesList(string const & countriesListFile, TCountriesTree & countries)
  {
    countries.Clear();
    ifstream stream(countriesListFile.c_str());
    std::string line;
    CountryPolygons * currentCountry = &countries.Value();
    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      // calculate spaces - depth inside the tree
      int spaces = 0;
      for (size_t i = 0; i < line.size(); ++i)
      {
        if (line[i] == ' ')
          ++spaces;
        else
          break;
      }
      switch (spaces)
      {
      case 0: // this is value for current tree node
        CHECK(false, ());
        break;
      case 1: // country group
      case 2: // country name
      case 3: // region
        currentCountry = &countries.AddAtDepth(spaces - 1, CountryPolygons(line.substr(spaces)));
        break;
      default:
        return false;
      }
    }
    return countries.SiblingsCount() > 0;
  }

}
