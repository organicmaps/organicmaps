#include "country_info.hpp"

#include "../indexer/geometry_serialization.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/read_write_utils.hpp"

#include "../base/string_utils.hpp"


namespace storage
{
  CountryInfoGetter::CountryInfoGetter(ModelReaderPtr reader)
    : m_reader(reader)
  {
    ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
    rw::Read(src, m_countries);
  }

  template <class ToDo>
  void CountryInfoGetter::ForEachCountry(m2::PointD const & pt, ToDo & toDo)
  {
    for (size_t i = 0; i < m_countries.size(); ++i)
      if (m_countries[i].m_rect.IsPointInside(pt))
        if (!toDo(i))
          return;
  }

  bool CountryInfoGetter::GetByPoint::operator() (size_t id)
  {
    ReaderSource<ModelReaderPtr> src(m_info.m_reader.GetReader(strings::to_string(id)));

    uint32_t const count = ReadVarUint<uint32_t>(src);
    for (size_t i = 0; i < count; ++i)
    {
      vector<m2::PointD> points;
      serial::LoadOuterPath(src, serial::CodingParams(), points);

      m2::RegionD rgn(points.begin(), points.end());
      if (rgn.Contains(m_pt))
      {
        m_res = id;
        return false;
      }
    }

    return true;
  }

  string CountryInfoGetter::GetRegionName(m2::PointD const & pt)
  {
    GetByPoint doGet(*this, pt);
    ForEachCountry(pt, doGet);

    if (doGet.m_res != -1)
    {
      return m_countries[doGet.m_res].m_name;
    }

    return string();
  }
}
