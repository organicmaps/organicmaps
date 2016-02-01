#pragma once
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace storage
{
  using TCountryId = string;
  using TCountriesSet = set<TCountryId>;
  using TCountriesVec = vector<TCountryId>;

  extern const storage::TCountryId kInvalidCountryId;

  // @TODO(bykoianko) Check in counrtry tree if the countryId valid.
  bool IsCountryIdValid(TCountryId const & countryId);

  struct TIndex
  {
    static int const INVALID;

    int m_group;
    int m_country;
    int m_region;

    TIndex(int group = INVALID, int country = INVALID, int region = INVALID)
      : m_group(group), m_country(country), m_region(region) {}

    bool IsValid() const { return (m_group != INVALID); }

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
} //  namespace storage
