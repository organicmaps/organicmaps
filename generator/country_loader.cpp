#include "country_loader.hpp"

#include "../base/std_serialization.hpp"
#include "../base/logging.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/streams_sink.hpp"

namespace borders
{
bool ReadPolygon(istream & stream, m2::RegionD & region, string const & filename)
{
  string line, name;
  double lon, lat;

  // read ring id, fail if it's empty
  getline(stream, name);
  if (name.empty() || name == "END")
    return false;

  while (stream.good())
  {
    getline(stream, line);
    strings::Trim(line);

    if (line.empty())
      continue;

    if (line == "END")
      break;

    istringstream iss(line);
    iss >> lon >> lat;
    CHECK(!iss.fail(), ("Incorrect data in", filename));

    region.AddPoint(MercatorBounds::FromLatLon(lat, lon));
  }

  // drop inner rings
  return name[0] != '!';
}

bool LoadBorders(string const & borderFile, vector<m2::RegionD> & outBorders)
{
  ifstream stream(borderFile);
  string line;
  if (!getline(stream, line).good()) // skip title
  {
    FileReader file(borderFile);
    ReaderSource<FileReader> source(file);
    stream::SinkReaderStream<ReaderSource<FileReader> > stream(source);

    stream >> outBorders;
    CHECK(!outBorders.empty(), ("Borders weren't loaded from", borderFile));
  }

  m2::RegionD currentRegion;
  while (ReadPolygon(stream, currentRegion, borderFile))
  {
    CHECK(currentRegion.IsValid(), ("Invalid region in", borderFile));
    outBorders.push_back(currentRegion);
    currentRegion = m2::RegionD();
  }

  CHECK(!outBorders.empty(), ("No borders were loaded from", borderFile));
  return true;
}

class PolygonLoader
{
public:
  PolygonLoader(string const & baseDir, CountriesContainerT & countries)
    : m_baseDir(baseDir), m_countries(countries) {}

  void operator() (string const & name)
  {
    if (m_polygons.m_name.empty())
      m_polygons.m_name = name;

    vector<m2::RegionD> borders;
    if (LoadBorders(m_baseDir + BORDERS_DIR + name + BORDERS_EXTENSION, borders))
    {
      for (auto const & border : borders)
      {
        m2::RectD const rect(border.GetRect());
        m_rect.Add(rect);
        m_polygons.m_regions.Add(border, rect);
      }
    }
  }

  void Finish()
  {
    if (!m_polygons.IsEmpty())
    {
      ASSERT_NOT_EQUAL ( m_rect, m2::RectD::GetEmptyRect(), () );
      m_countries.Add(m_polygons, m_rect);
    }

    m_polygons.Clear();
    m_rect.MakeEmpty();
  }

private:
  CountryPolygons m_polygons;
  m2::RectD m_rect;

  string const & m_baseDir;
  CountriesContainerT & m_countries;
};

bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries)
{
  countries.Clear();

  LOG(LINFO, ("Loading countries..."));

  PolygonLoader loader(baseDir, countries);
  ForEachCountry(baseDir, loader);

  LOG(LINFO, ("Countries loaded:", countries.GetSize()));

  return !countries.IsEmpty();
}

}
