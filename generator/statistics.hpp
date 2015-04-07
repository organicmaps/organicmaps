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

  template <class TKey>
  struct GeneralInfoKey
  {
    TKey m_key;
    GeneralInfo m_info;

    GeneralInfoKey(TKey key) : m_key(key) {}

    bool operator< (GeneralInfoKey const & rhs) const
    {
      return m_key < rhs.m_key;
    }
  };

  struct TypeTag
  {
    uint32_t m_val;

    TypeTag(uint32_t v) : m_val(v) {}

    bool operator< (TypeTag const & rhs) const
    {
      return m_val < rhs.m_val;
    }
  };

  struct MapInfo
  {
    set<GeneralInfoKey<feature::EGeomType> > m_byGeomType;
    set<GeneralInfoKey<TypeTag> > m_byClassifType;
    set<GeneralInfoKey<uint32_t> > m_byPointsCount, m_byTrgCount;

    GeneralInfo m_inner[3];

    template <class TKey, class TSet>
    void AddToSet(TKey key, uint32_t sz, TSet & theSet)
    {
      if (sz > 0)
      {
        // GCC doesn't allow to modify set value ...
        const_cast<GeneralInfo &>(
            theSet.insert(GeneralInfoKey<TKey>(key)).first->m_info).Add(sz);
      }
    }
  };

  void FileContainerStatistic(string const & fPath);

  void CalcStatistic(string const & fPath, MapInfo & info);
  void PrintStatistic(MapInfo & info);
}
