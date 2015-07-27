#pragma once

#include "storage/country_decl.hpp"
#include "storage/simple_tree.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "platform/country_defines.hpp"

#include "defines.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace update
{
class SizeUpdater;
}

namespace storage
{
/// Serves as a proxy between GUI and downloaded files
class Country
{
  friend class update::SizeUpdater;
  /// Name in the country node tree
  string m_name;
  /// Flag to display
  string m_flag;
  /// stores squares with world pieces which are part of the country
  buffer_vector<platform::CountryFile, 1> m_files;

public:
  Country() {}
  Country(string const & name, string const & flag = "") : m_name(name), m_flag(flag) {}

  bool operator<(Country const & other) const { return Name() < other.Name(); }

  void AddFile(platform::CountryFile const & file);

  size_t GetFilesCount() const { return m_files.size(); }

  /// This function valid for current logic - one file for one country (region).
  /// If the logic will be changed, replace GetFile with ForEachFile.
  platform::CountryFile const & GetFile() const
  {
    ASSERT_EQUAL(m_files.size(), 1, (m_name));
    return m_files.front();
  }

  string const & Name() const { return m_name; }
  string const & Flag() const { return m_flag; }

  uint64_t Size(MapOptions opt) const;
};

typedef SimpleTree<Country> CountriesContainerT;

/// @return version of country file or -1 if error was encountered
int64_t LoadCountries(string const & jsonBuffer, CountriesContainerT & countries);

void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info);

void LoadCountryCode2File(string const & jsonBuffer, multimap<string, string> & code2file);

bool SaveCountries(int64_t version, CountriesContainerT const & countries, string & jsonBuffer);
}  // namespace storage
