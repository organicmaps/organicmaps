#pragma once

#include "storage/storage_defines.hpp"

#include "geometry/rect2d.hpp"

#include <string>

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
    CountryInfo(std::string const & id) : m_name(id) {}

    /// Name (in native language) of country or region.
    /// (if empty - equals to file name of country - no additional memory)
    std::string m_name;

    bool IsNotEmpty() const { return !m_name.empty(); }

    // @TODO(bykoianko) Twine will be used intead of this function.
    // So id (fName) will be converted to a local name.
    static void FileName2FullName(std::string & fName);
    static void FullName2GroupAndMap(std::string const & fName, std::string & group,
                                     std::string & map);
  };
}
