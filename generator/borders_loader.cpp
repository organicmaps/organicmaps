#include "generator/borders_loader.hpp"
#include "generator/borders_generator.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "storage/country_polygon.hpp"

#include "indexer/geometry_serialization.hpp"
#include "indexer/scales.hpp"
#include "geometry/mercator.hpp"

#include "geometry/simplification.hpp"
#include "geometry/distance.hpp"

#include "coding/file_container.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/vector.hpp"
#include "std/bind.hpp"


namespace borders
{

class PolygonLoader
{
  CountryPolygons m_polygons;
  m2::RectD m_rect;

  CountriesContainerT & m_countries;

public:
  PolygonLoader(CountriesContainerT & countries)
    : m_countries(countries) {}

  void operator() (string const & name, vector<m2::RegionD> const & borders)
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
      ASSERT_NOT_EQUAL ( m_rect, m2::RectD::GetEmptyRect(), () );
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
  CHECK(Platform::IsFileExistsByFullPath(bordersDir), ("Cannot read borders directory", bordersDir));

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

bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries)
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
  FilesContainerW m_writer;

  vector<storage::CountryDef> m_polys;

public:
  PackedBordersGenerator(string const & baseDir)
    : m_writer(baseDir + PACKED_POLYGONS_FILE)
  {
  }

  void operator() (string const & name, vector<m2::RegionD> const & borders)
  {
    // use index in vector as tag
    FileWriter w = m_writer.GetWriter(strings::to_string(m_polys.size()));
    serial::CodingParams cp;

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
      typedef vector<m2::PointD> VectorT;
      typedef m2::DistanceToLineSquare<m2::PointD> DistanceT;

      VectorT const & in = border.Data();
      VectorT out;

      /// @todo Choose scale level for simplification.
      double const eps = my::sq(scales::GetEpsilonForSimplify(10));
      DistanceT dist;
      SimplifyNearOptimal(20, in.begin(), in.end(), eps, dist,
                          AccumulateSkipSmallTrg<DistanceT, m2::PointD>(dist, out, eps));

      serial::SaveOuterPath(out, cp, w);
    }
  }

  void Finish() {}

  void WritePolygonsInfo()
  {
    FileWriter w = m_writer.GetWriter(PACKED_POLYGONS_INFO_TAG);
    rw::Write(w, m_polys);
  }
};

void GeneratePackedBorders(string const & baseDir)
{
  PackedBordersGenerator generator(baseDir);
  ForEachCountry(baseDir, generator);
  generator.WritePolygonsInfo();
}

} // namespace borders
