#pragma once

#include "storage/country_decl.hpp"
#include "storage/index.hpp"
#include "storage/simple_tree.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "platform/country_defines.hpp"

#include "defines.hpp"

#include "geometry/rect2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace update
{
class SizeUpdater;
}

namespace storage
{
using TMapping = map<TCountryId, TCountriesSet>;

/// Serves as a proxy between GUI and downloaded files
class Country
{
  friend class update::SizeUpdater;
  /// Name in the country node tree
  TCountryId m_name;
  /// |m_file| is a CountryFile of mwm with id == |m_name|.
  /// if |m_name| is node id of a group of mwms, |m_file| is empty.
  platform::CountryFile m_file;
  /// The number of descendant mwm files of |m_name|. Only files (leaves in tree) are counted.
  /// If |m_name| is a mwm file name |m_childrenNumber| == 1.
  uint32_t m_subtreeMwmNumber;
  /// Size of descendant mwm files of |m_name|.
  /// If |m_name| is a mwm file name |m_subtreeMwmSizeBytes| is equal to size of the mwm.
  size_t m_subtreeMwmSizeBytes;

public:
  Country() = default;
  Country(TCountryId const & name) : m_name(name) {}

  bool operator<(Country const & other) const { return Name() < other.Name(); }
  void SetFile(platform::CountryFile const & file) { m_file = file; }
  void SetSubtreeAttrs(uint32_t subtreeMwmNumber, size_t subtreeMwmSizeBytes)
  {
    m_subtreeMwmNumber = subtreeMwmNumber;
    m_subtreeMwmSizeBytes = subtreeMwmSizeBytes;
  }
  uint32_t GetSubtreeMwmCounter() const { return m_subtreeMwmNumber; }
  size_t GetSubtreeMwmSizeBytes() const { return m_subtreeMwmSizeBytes; }

  /// This function valid for current logic - one file for one country (region).
  /// If the logic will be changed, replace GetFile with ForEachFile.
  platform::CountryFile const & GetFile() const { return m_file; }
  TCountryId const & Name() const { return m_name; }
};

typedef SimpleTree<Country> TCountriesContainer;

/// @return version of country file or -1 if error was encountered
int64_t LoadCountries(string const & jsonBuffer, TCountriesContainer & countries, TMapping * mapping = nullptr);

void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info,
                                 bool & isSingleMwm);
}  // namespace storage
