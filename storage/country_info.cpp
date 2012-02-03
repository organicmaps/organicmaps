#include "country_info.hpp"
#include "country_polygon.hpp"
#include "country.hpp"

#include "../indexer/geometry_serialization.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/read_write_utils.hpp"

#include "../base/string_utils.hpp"


namespace storage
{
  CountryInfoGetter::CountryInfoGetter(ModelReaderPtr polyR, ModelReaderPtr countryR)
    : m_reader(polyR), m_cache(2)
  {
    ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
    rw::Read(src, m_countries);

    string buffer;
    countryR.ReadAsString(buffer);
    LoadCountryFile2CountryInfo(buffer, m_id2info);
  }

  template <class ToDo>
  void CountryInfoGetter::ForEachCountry(m2::PointD const & pt, ToDo & toDo) const
  {
    for (size_t i = 0; i < m_countries.size(); ++i)
      if (m_countries[i].m_rect.IsPointInside(pt))
        if (!toDo(i))
          return;
  }

  bool CountryInfoGetter::GetByPoint::operator() (size_t id)
  {
    vector<m2::RegionD> const & rgnV = m_info.GetRegions(id);

    for (size_t i = 0; i < rgnV.size(); ++i)
    {
      if (rgnV[i].Contains(m_pt))
      {
        m_res = id;
        return false;
      }
    }

    return true;
  }

  vector<m2::RegionD> const & CountryInfoGetter::GetRegions(size_t id) const
  {
    bool isFound = false;
    vector<m2::RegionD> & rgnV = m_cache.Find(id, isFound);

    if (!isFound)
    {
      rgnV.clear();

      // load regions from file
      ReaderSource<ModelReaderPtr> src(m_reader.GetReader(strings::to_string(id)));

      uint32_t const count = ReadVarUint<uint32_t>(src);
      for (size_t i = 0; i < count; ++i)
      {
        vector<m2::PointD> points;
        serial::LoadOuterPath(src, serial::CodingParams(), points);

        rgnV.push_back(m2::RegionD());
        rgnV.back().Assign(points.begin(), points.end());
      }
    }

    return rgnV;
  }

  string CountryInfoGetter::GetRegionFile(m2::PointD const & pt) const
  {
    GetByPoint doGet(*this, pt);
    ForEachCountry(pt, doGet);

    if (doGet.m_res != -1)
      return m_countries[doGet.m_res].m_name;
    else
      return string();
  }

  void CountryInfoGetter::GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const
  {
    GetByPoint doGet(*this, pt);
    ForEachCountry(pt, doGet);

    if (doGet.m_res != -1)
      GetRegionInfo(m_countries[doGet.m_res].m_name, info);
  }

  void CountryInfoGetter::GetRegionInfo(string const & id, CountryInfo & info) const
  {
    map<string, CountryInfo>::const_iterator i = m_id2info.find(id);

    // Take into account 'minsk-pass'.
    if (i == m_id2info.end()) return;
    //ASSERT ( i != m_id2info.end(), () );

    info = i->second;

    if (info.m_name.empty())
      info.m_name = id;

    if (id.find('_') != string::npos)
    {
      size_t const i = info.m_name.find('_');
      ASSERT ( i != string::npos, () );

      // replace '_' with ", "
      info.m_name[i] = ',';
      info.m_name.insert(i+1, " ");
    }
  }

  void CountryInfoGetter::CalcUSALimitRect(m2::RectD rects[3]) const
  {
    for (size_t i = 0; i < m_countries.size(); ++i)
    {
      if (m_countries[i].m_name.find("USA_") == 0)
      {
        if (m_countries[i].m_name == "USA_Alaska")
          rects[1] = m_countries[i].m_rect;
        else if (m_countries[i].m_name == "USA_Hawaii")
          rects[2] = m_countries[i].m_rect;
        else
          rects[0].Add(m_countries[i].m_rect);
      }
    }
  }

  m2::RectD CountryInfoGetter::CalcLimitRect(string const & prefix) const
  {
    m2::RectD r;
    for (size_t i = 0; i < m_countries.size(); ++i)
    {
      if (m_countries[i].m_name.find(prefix) == 0)
        r.Add(m_countries[i].m_rect);
    }
    return r;
  }
}
