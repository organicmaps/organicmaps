#pragma once
#include "std/string.hpp"


namespace storage
{
  struct TIndex
  {
    static int const INVALID;

    int m_group;
    int m_country;
    int m_region;

    TIndex(int group = INVALID, int country = INVALID, int region = INVALID)
      : m_group(group), m_country(country), m_region(region) {}

    bool IsValid() const { return (m_group != INVALID && m_country != INVALID); }

    bool operator==(TIndex const & other) const
    {
      return (m_group == other.m_group &&
              m_country == other.m_country &&
              m_region == other.m_region);
    }
     
    bool operator!=(TIndex const & other) const
    {
      return !(*this == other);
    }

    bool operator<(TIndex const & other) const
    {
      if (m_group != other.m_group)
        return m_group < other.m_group;
      else if (m_country != other.m_country)
        return m_country < other.m_country;
      return m_region < other.m_region;
    }
  };

  string DebugPrint(TIndex const & r);
}
