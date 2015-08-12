#pragma once

#include "indexer/feature.hpp"

#include "std/map.hpp"


namespace stats
{
  struct GeneralInfo
  {
    uint64_t m_count, m_size;

    GeneralInfo() : m_count(0), m_size(0) {}

    void Add(uint64_t sz)
    {
      if (sz > 0)
      {
        ++m_count;
        m_size += sz;
      }
    }
  };

  template <class T, int Tag>
  struct IntegralType
  {
    T m_val;
    explicit IntegralType(T v) : m_val(v) {}
    bool operator<(IntegralType const & rhs) const { return m_val < rhs.m_val; }
  };

  using ClassifType = IntegralType<uint32_t, 0>;
  using CountType = IntegralType<uint32_t, 1>;
  using AreaType = IntegralType<size_t, 2>;

  struct MapInfo
  {
    map<feature::EGeomType, GeneralInfo> m_byGeomType;
    map<ClassifType, GeneralInfo> m_byClassifType;
    map<CountType, GeneralInfo> m_byPointsCount, m_byTrgCount;
    map<AreaType, GeneralInfo> m_byAreaSize;

    GeneralInfo m_inner[3];

    template <class TKey, class TSet>
    void AddToSet(TKey key, uint32_t sz, TSet & theSet)
    {
      if (sz > 0)
        theSet[key].Add(sz);
    }
  };

  void FileContainerStatistic(string const & fPath);

  void CalcStatistic(string const & fPath, MapInfo & info);
  void PrintStatistic(MapInfo & info);
}
