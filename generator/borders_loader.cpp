#include "generator/borders_loader.hpp"

#include "generator/borders_generator.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "storage/country_polygon.hpp"

#include "indexer/scales.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/simplification.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

namespace borders
{
class PolygonLoader
{
  CountryPolygons m_polygons;
  m2::RectD m_rect;

  CountriesContainer & m_countries;

public:
  PolygonLoader(CountriesContainer & countries) : m_countries(countries) {}

  void operator()(string const & name, vector<m2::RegionD> const & borders)
  {
    if (m_polygons.m_name.empty())
      m_polygons.m_name = name;

    for (m2::RegionD const & border : borders)
    {
      m2::RectD const rect(border.GetRect());
      m_rect.Add(rect);
      m_polygons.m_regions.Add(border, rect);
    }
  }

  void Finish()
  {
    if (!m_polygons.IsEmpty())
    {
      ASSERT_NOT_EQUAL(m_rect, m2::RectD::GetEmptyRect(), ());
      m_countries.Add(m_polygons, m_rect);
    }

    m_polygons.Clear();
    m_rect.MakeEmpty();
  }
};

template <class ToDo>
void ForEachCountry(string const & baseDir, ToDo & toDo)
{
  string const bordersDir = baseDir + BORDERS_DIR;
  CHECK(Platform::IsFileExistsByFullPath(bordersDir),
        ("Cannot read borders directory", bordersDir));

  Platform::FilesList files;
  Platform::GetFilesByExt(bordersDir, BORDERS_EXTENSION, files);
  for (string file : files)
  {
    vector<m2::RegionD> borders;
    if (osm::LoadBorders(bordersDir + file, borders))
    {
      my::GetNameWithoutExt(file);
      toDo(file, borders);
      toDo.Finish();
    }
  }
}

bool LoadCountriesList(string const & baseDir, CountriesContainer & countries)
{
  countries.Clear();

  LOG(LINFO, ("Loading countries."));

  PolygonLoader loader(countries);
  ForEachCountry(baseDir, loader);

  LOG(LINFO, ("Countries loaded:", countries.GetSize()));

  return !countries.IsEmpty();
}

class PackedBordersGenerator
{
public:
  explicit PackedBordersGenerator(string const & baseDir) : m_writer(baseDir + PACKED_POLYGONS_FILE)
  {
  }

  void operator()(string const & name, vector<m2::RegionD> const & borders)
  {
    // use index in vector as tag
    FileWriter w = m_writer.GetWriter(strings::to_string(m_polys.size()));
    serial::GeometryCodingParams cp;

    // calc rect
    m2::RectD rect;
    for (m2::RegionD const & border : borders)
      rect.Add(border.GetRect());

    // store polygon info
    m_polys.push_back(storage::CountryDef(name, rect));

    // write polygons as paths
    WriteVarUint(w, borders.size());
    for (m2::RegionD const & border : borders)
    {
      vector<m2::PointD> const & in = border.Data();
      vector<m2::PointD> out;

      /// @todo Choose scale level for simplification.
      double const eps = pow(scales::GetEpsilonForSimplify(10), 2);
      m2::SquaredDistanceFromSegmentToPoint<m2::PointD> distFn;
      SimplifyNearOptimal(20, in.begin(), in.end(), eps, distFn,
                          AccumulateSkipSmallTrg<decltype(distFn), m2::PointD>(distFn, out, eps));

      serial::SaveOuterPath(out, cp, w);
    }
  }

  void Finish() {}

  void WritePolygonsInfo()
  {
    FileWriter w = m_writer.GetWriter(PACKED_POLYGONS_INFO_TAG);
    rw::Write(w, m_polys);
  }

private:
  FilesContainerW m_writer;

  vector<storage::CountryDef> m_polys;
};

void GeneratePackedBorders(string const & baseDir)
{
  PackedBordersGenerator generator(baseDir);
  ForEachCountry(baseDir, generator);
  generator.WritePolygonsInfo();
}

void UnpackBorders(string const & baseDir, string const & targetDir)
{
  if (!Platform::IsFileExistsByFullPath(targetDir) && !Platform::MkDirChecked(targetDir))
    MYTHROW(FileSystemException, ("Unable to find or create directory", targetDir));

  vector<storage::CountryDef> countries;
  FilesContainerR reader(my::JoinFoldersToPath(baseDir, PACKED_POLYGONS_FILE));
  ReaderSource<ModelReaderPtr> src(reader.GetReader(PACKED_POLYGONS_INFO_TAG));
  rw::Read(src, countries);

  for (size_t id = 0; id < countries.size(); id++)
  {
    ofstream poly(my::JoinFoldersToPath(targetDir, countries[id].m_countryId + ".poly"));
    poly << countries[id].m_countryId << endl;
    src = reader.GetReader(strings::to_string(id));
    uint32_t const count = ReadVarUint<uint32_t>(src);
    for (size_t i = 0; i < count; ++i)
    {
      poly << i + 1 << endl;
      vector<m2::PointD> points;
      serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
      for (auto p : points)
      {
        ms::LatLon const ll = MercatorBounds::ToLatLon(p);
        poly << "    " << scientific << ll.lon << "    " << ll.lat << endl;
      }
      poly << "END" << endl;
    }
    poly << "END" << endl;
    poly.close();
  }
}

bool GetBordersRect(string const & baseDir, string const & country, m2::RectD & bordersRect)
{
  string const bordersFile = my::JoinPath(baseDir, BORDERS_DIR, country + BORDERS_EXTENSION);
  if (!Platform::IsFileExistsByFullPath(bordersFile))
  {
    LOG(LWARNING, ("File with borders does not exist:", bordersFile));
    return false;
  }

  vector<m2::RegionD> borders;
  CHECK(osm::LoadBorders(bordersFile, borders), ());
  bordersRect.MakeEmpty();
  for (auto const & border : borders)
    bordersRect.Add(border.GetRect());

  return true;
}
}  // namespace borders
