#pragma once

#include "../base/std_serialization.hpp"

#include "../coding/streams_sink.hpp"

#include "../geometry/rect2d.hpp"

#include "../indexer/defines.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

class Reader;
class Writer;

namespace mapinfo
{
  typedef pair<string, uint64_t> TUrl;
  typedef vector<TUrl> TUrlContainer;

  /// helper function
  string FileNameFromUrl(string const & url);
  bool IsFileDownloaded(TUrl const & url);
  bool IsDatFile(string const & fileName);

  /// Serves as a proxy between GUI and downloaded files
  class Country
  {
    template <class TArchive> friend TArchive & operator << (TArchive & ar, mapinfo::Country const & country);
    template <class TArchive> friend TArchive & operator >> (TArchive & ar, mapinfo::Country & country);

  private:
    /// Europe, Asia etc.
    string m_group;
    /// USA, Switzerland etc.
    string m_country;
    /// can be empty, Alaska, California etc.
    string m_region;
    /// stores squares with world pieces where this country resides
    TUrlContainer m_urls;

  public:
    Country() {}
    Country(string const & group, string const & country, string const & region)
      : m_group(group), m_country(country), m_region(region) {}

    bool operator<(Country const & other) const { return Name() < other.Name(); }
    bool operator==(Country const & other) const
    {
      return m_group == other.m_group && m_country == other.m_country && m_region == other.m_region
          && m_urls == other.m_urls;
    }
    bool operator!=(Country const & other) const { return !(*this == other); }

    void AddUrl(TUrl const & url);
    TUrlContainer const & Urls() const { return m_urls; }

    string const & Group() const { return m_group; }
    string Name() const { return m_region.empty() ? m_country : m_country + " - " + m_region; }

    /// @return bounds for downloaded parts of the country or empty rect
    m2::RectD Bounds() const;
    /// Downloaded parts size
    uint64_t LocalSize() const;
    /// Not downloaded parts size
    uint64_t RemoteSize() const;
  };

  template <class TArchive> TArchive & operator >> (TArchive & ar, mapinfo::Country & country)
  {
    ar >> country.m_group;
    ar >> country.m_country;
    ar >> country.m_region;
    ar >> country.m_urls;
    return ar;
  }

  /// key is Country::Group()
  typedef map<string, vector<Country> > TCountriesContainer;

  /// @return false if new application version should be downloaded
  bool LoadCountries(TCountriesContainer & countries, string const & updateFile);
  void SaveCountries(TCountriesContainer const & countries, Writer & writer);
}
