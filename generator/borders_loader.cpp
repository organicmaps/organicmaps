#include "borders_loader.hpp"
#include "borders_generator.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/fstream.hpp"
#include "../std/vector.hpp"

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".borders"
#define POLYGONS_FILE "polygons.lst"

namespace borders
{
  class PolygonLoader
  {
    string m_baseDir;
    CountryPolygons & m_polygons;
    m2::RectD & m_rect;

  public:
    PolygonLoader(string const & basePolygonsDir, CountryPolygons & polygons, m2::RectD & rect)
      : m_baseDir(basePolygonsDir), m_polygons(polygons), m_rect(rect)
    {
    }

    void operator()(string const & name)
    {
      if (m_polygons.m_name.empty())
        m_polygons.m_name = name;

      vector<m2::RegionD> coutryBorders;
      if (osm::LoadBorders(m_baseDir + BORDERS_DIR + name + BORDERS_EXTENSION, coutryBorders))
      {
        for (size_t i = 0; i < coutryBorders.size(); ++i)
        {
          m2::RectD const rect(coutryBorders[i].GetRect());
          m_rect.Add(rect);
          m_polygons.m_regions.Add(coutryBorders[i], rect);
        }
      }
    }
  };

  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries)
  {
    countries.Clear();
    ifstream stream((baseDir + POLYGONS_FILE).c_str());
    string line;
    LOG(LINFO, ("Loading countries."));
    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      CountryPolygons country;
      m2::RectD rect;

      PolygonLoader loader(baseDir, country, rect);
      strings::Tokenize(line, "|", loader);
      if (!country.m_regions.IsEmpty())
        countries.Add(country, rect);
    }
    LOG(LINFO, ("Countries loaded:", countries.GetSize()));
    return !countries.IsEmpty();
  }

}
