#pragma once

#include "storage/index.hpp"

#include "geometry/rect2d.hpp"

#include "std/string.hpp"

namespace storage
{
  struct CountryDef
  {
    /// File name without extension (equal to english name - used in search for region).
    TCountryId m_countryId;
    m2::RectD m_rect;

    CountryDef() {}
    CountryDef(TCountryId const & countryId, m2::RectD const & rect)
      : m_countryId(countryId), m_rect(rect)
    {
    }
  };

  struct CountryInfo
  {
    CountryInfo() = default;
    CountryInfo(string const & id) : m_name(id) {}

    /// Name (in native language) of country or region.
    /// (if empty - equals to file name of country - no additional memory)
    string m_name;

    bool IsNotEmpty() const { return !m_name.empty(); }

    // @TODO(bykoianko) Twine will be used intead of this function.
    // So id (fName) will be converted to a local name.
    static void FileName2FullName(string & fName);
    static void FullName2GroupAndMap(string const & fName, string & group, string & map);
  };
}
