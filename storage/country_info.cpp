#include "country_info.hpp"
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
    LoadCountryFile2Name(buffer, m_id2name);
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

  string CountryInfoGetter::GetRegionName(m2::PointD const & pt) const
  {
    GetByPoint doGet(*this, pt);
    ForEachCountry(pt, doGet);

    if (doGet.m_res != -1)
      return GetRegionName(m_countries[doGet.m_res].m_name);

    return string();
  }

  string CountryInfoGetter::GetRegionName(string const & id) const
  {
    string name;

    map<string, string>::const_iterator i = m_id2name.find(id);
    if (i != m_id2name.end())
      name = i->second;
    else
      name = id;

    /// @todo Correct replace '_' with ", " in name.
    if (id.find_first_of('_') != string::npos)
    {
      // I don't know how to do it best for UTF8, but this variant will work
      // correctly for now (utf8-names of compound countries are equal to ascii-names).

      size_t const i = name.find_first_of('_');
      ASSERT_NOT_EQUAL ( i, string::npos, () );
      name = name.substr(0, i) + ", " + name.substr(i+1);
    }

    return name;
  }
}
