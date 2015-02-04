#include "borders_loader.hpp"
#include "borders_generator.hpp"

#include "../defines.hpp"

#include "../storage/country_polygon.hpp"

#include "../indexer/geometry_serialization.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/mercator.hpp"

#include "../geometry/simplification.hpp"
#include "../geometry/distance.hpp"

#include "../coding/file_container.hpp"
#include "../coding/read_write_utils.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/fstream.hpp"
#include "../std/vector.hpp"
#include "../std/bind.hpp"

namespace borders
{

class PackedBordersGenerator
{
  FilesContainerW m_writer;
  string const & m_baseDir;

  vector<storage::CountryDef> m_polys;

public:
  PackedBordersGenerator(string const & baseDir)
    : m_writer(baseDir + PACKED_POLYGONS_FILE), m_baseDir(baseDir)
  {
  }

  void operator() (string const & name)
  {
    vector<m2::RegionD> borders;
    if (borders::LoadBorders(m_baseDir + BORDERS_DIR + name + BORDERS_EXTENSION, borders))
    {
      // use index in vector as tag
      FileWriter w = m_writer.GetWriter(strings::to_string(m_polys.size()));
      serial::CodingParams cp;

      uint32_t const count = static_cast<uint32_t>(borders.size());

      // calc rect
      m2::RectD rect;
      for (uint32_t i = 0; i < count; ++i)
        rect.Add(borders[i].GetRect());

      // store polygon info
      m_polys.push_back(storage::CountryDef(name, rect));

      // write polygons as paths
      WriteVarUint(w, count);
      for (uint32_t i = 0; i < count; ++i)
      {
        typedef vector<m2::PointD> VectorT;
        typedef m2::DistanceToLineSquare<m2::PointD> DistanceT;

        VectorT const & in = borders[i].Data();
        VectorT out;

        /// @todo Choose scale level for simplification.
        double const eps = my::sq(scales::GetEpsilonForSimplify(10));
        DistanceT dist;
        SimplifyNearOptimal(20, in.begin(), in.end(), eps, dist,
                            AccumulateSkipSmallTrg<DistanceT, m2::PointD>(dist, out, eps));

        serial::SaveOuterPath(out, cp, w);
      }
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
