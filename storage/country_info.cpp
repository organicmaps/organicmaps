#include "storage/country_info.hpp"
#include "storage/country_polygon.hpp"
#include "storage/country.hpp"

#include "indexer/geometry_serialization.hpp"

#include "geometry/region2d.hpp"

#include "coding/read_write_utils.hpp"

#include "base/string_utils.hpp"


namespace storage
{
  /*
  class LessCountryDef
  {
    bool CompareStrings(string const & s1, string const & s2) const
    {
      // Do this stuff because of 'Guinea-Bissau.mwm' goes before 'Guinea.mwm'
      // in file system (and in PACKED_POLYGONS_FILE also).
      size_t n = min(s1.size(), s2.size());
      return lexicographical_compare(s1.begin(), s1.begin() + n,
                                     s2.begin(), s2.begin() + n);
    }

  public:
    bool operator() (CountryDef const & r1, string const & r2) const
    {
      return CompareStrings(r1.m_name, r2);
    }
    bool operator() (string const & r1, CountryDef const & r2) const
    {
      return CompareStrings(r1, r2.m_name);
    }
    bool operator() (CountryDef const & r1, CountryDef const & r2) const
    {
      return CompareStrings(r1.m_name, r2.m_name);
    }
  };
  */

  CountryInfoGetter::CountryInfoGetter(ModelReaderPtr polyR, ModelReaderPtr countryR)
    : m_reader(polyR), m_cache(3)
  {
    ReaderSource<ModelReaderPtr> src(m_reader.GetReader(PACKED_POLYGONS_INFO_TAG));
    rw::Read(src, m_countries);

/*
    // We can't change the order of countries.
#ifdef DEBUG
    LessCountryDef check;
    for (size_t i = 0; i < m_countries.size() - 1; ++i)
      ASSERT ( !check(m_countries[i+1], m_countries[i]),
               (m_countries[i].m_name, m_countries[i+1].m_name) );
#endif
*/

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
    vector<m2::RegionD> & rgnV = m_cache.Find(static_cast<uint32_t>(id), isFound);

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
        rgnV.emplace_back(points.begin(), points.end());
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
    auto i = m_id2info.find(id);

    // Take into account 'minsk-pass'.
    if (i == m_id2info.end()) return;
    //ASSERT ( i != m_id2info.end(), () );

    info = i->second;

    if (info.m_name.empty())
      info.m_name = id;

    CountryInfo::FileName2FullName(info.m_name);
  }

  template <class ToDo> void CountryInfoGetter::ForEachCountry(string const & prefix, ToDo toDo) const
  {
    for (auto const & c : m_countries)
    {
      if (c.m_name.find(prefix) == 0)
        toDo(c);
    }

    /// @todo Store sorted list of polygons in PACKED_POLYGONS_FILE.
    /*
    pair<IterT, IterT> r = equal_range(m_countries.begin(), m_countries.end(), file, LessCountryDef());
    while (r.first != r.second)
    {
      toDo(r.first->m_name);
      ++r.first;
    }
    */
  }

  namespace
  {
    class DoCalcUSA
    {
      m2::RectD * m_rects;
    public:
      DoCalcUSA(m2::RectD * rects) : m_rects(rects) {}
      void operator() (CountryDef const & c)
      {
        if (c.m_name == "USA_Alaska")
          m_rects[1] = c.m_rect;
        else if (c.m_name == "USA_Hawaii")
          m_rects[2] = c.m_rect;
        else
          m_rects[0].Add(c.m_rect);
      }
    };

    void AddRect(m2::RectD & r, CountryDef const & c)
    {
      r.Add(c.m_rect);
    }
  }

  void CountryInfoGetter::CalcUSALimitRect(m2::RectD rects[3]) const
  {
    ForEachCountry("USA_", DoCalcUSA(rects));
  }

  m2::RectD CountryInfoGetter::CalcLimitRect(string const & prefix) const
  {
    m2::RectD r;
    ForEachCountry(prefix, bind(&AddRect, ref(r), _1));
    return r;
  }

  void CountryInfoGetter::GetMatchedRegions(string const & enName, IDSet & regions) const
  {
    for (size_t i = 0; i < m_countries.size(); ++i)
    {
      /// Match english name with region file name (they are equal in almost all cases).
      /// @todo Do it smarter in future.
      string s = m_countries[i].m_name;
      strings::AsciiToLower(s);
      if (s.find(enName) != string::npos)
        regions.push_back(i);
    }
  }

  bool CountryInfoGetter::IsBelongToRegion(m2::PointD const & pt, IDSet const & regions) const
  {
    GetByPoint doCheck(*this, pt);
    for (size_t i = 0; i < regions.size(); ++i)
      if (m_countries[regions[i]].m_rect.IsPointInside(pt) && !doCheck(regions[i]))
        return true;

    return false;
  }

  bool CountryInfoGetter::IsBelongToRegion(string const & fileName, IDSet const & regions) const
  {
    for (size_t i = 0; i < regions.size(); ++i)
    {
      if (m_countries[regions[i]].m_name == fileName)
        return true;
    }

    return false;
  }

namespace
{
  class DoFreeCacheMemory
  {
  public:
    void operator() (vector<m2::RegionD> & v) const
    {
      vector<m2::RegionD> emptyV;
      emptyV.swap(v);
    }
  };
}

  void CountryInfoGetter::ClearCaches() const
  {
    m_cache.ForEachValue(DoFreeCacheMemory());
    m_cache.Reset();
  }
}
