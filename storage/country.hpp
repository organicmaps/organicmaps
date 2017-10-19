#pragma once

#include "storage/country_decl.hpp"
#include "storage/country_tree.hpp"
#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "platform/country_defines.hpp"

#include "defines.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace update
{
class SizeUpdater;
}

namespace storage
{
using TMappingOldMwm = std::map<TCountryId, TCountriesSet>;
/// Map from key affiliation words into MWM IDs (file names).
using TMappingAffiliations = std::unordered_map<string, vector<TCountryId>>;

/// This class keeps all the information about a country in country tree (TCountryTree).
/// It is guaranteed that every node represent a unique region has a unique |m_name| in country
/// tree.
/// If several nodes have the same |m_name| they represent the same region.
/// It happends in case of disputed territories.
/// That means that
/// * if several leaf nodes have the same |m_name| one mwm file corresponds
/// to all of them
/// * if several expandable (not leaf) nodes have the same |m_name|
/// the same hierarchy, the same set of mwm files and the same attributes correspond to all of them.
/// So in most cases it's enough to find the first inclusion of |Country| in country tree.
class Country
{
  friend class update::SizeUpdater;
  /// Name in the country node tree. In single mwm case it's a country id.
  TCountryId m_name;
  /// Country id of parent of m_name in country tree. m_parent == kInvalidCountryId for the root.
  TCountryId m_parent;
  /// |m_file| is a CountryFile of mwm with id == |m_name|.
  /// if |m_name| is node id of a group of mwms, |m_file| is empty.
  platform::CountryFile m_file;
  /// The number of descendant mwm files of |m_name|. Only files (leaves in tree) are counted.
  /// If |m_name| is a mwm file name |m_childrenNumber| == 1.
  TMwmCounter m_subtreeMwmNumber;
  /// Size of descendant mwm files of |m_name|.
  /// If |m_name| is a mwm file name |m_subtreeMwmSizeBytes| is equal to size of the mwm.
  TMwmSize m_subtreeMwmSizeBytes;

public:
  Country() = default;
  explicit Country(TCountryId const & name, TCountryId const & parent = kInvalidCountryId)
    : m_name(name), m_parent(parent) {}

  void SetFile(platform::CountryFile const & file) { m_file = file; }
  void SetSubtreeAttrs(TMwmCounter subtreeMwmNumber, TMwmSize subtreeMwmSizeBytes)
  {
    m_subtreeMwmNumber = subtreeMwmNumber;
    m_subtreeMwmSizeBytes = subtreeMwmSizeBytes;
  }
  TMwmCounter GetSubtreeMwmCounter() const { return m_subtreeMwmNumber; }
  TMwmSize GetSubtreeMwmSizeBytes() const { return m_subtreeMwmSizeBytes; }
  TCountryId GetParent() const { return m_parent; }

  /// This function valid for current logic - one file for one country (region).
  /// If the logic will be changed, replace GetFile with ForEachFile.
  platform::CountryFile const & GetFile() const { return m_file; }
  TCountryId const & Name() const { return m_name; }
};

using TCountryTree = CountryTree<TCountryId, Country>;
using TCountryTreeNode = TCountryTree::Node;

/// @return version of country file or -1 if error was encountered
int64_t LoadCountriesFromBuffer(std::string const & buffer, TCountryTree & countries,
                                TMappingAffiliations & affiliations,
                                TMappingOldMwm * mapping = nullptr);
int64_t LoadCountriesFromFile(std::string const & path, TCountryTree & countries,
                              TMappingAffiliations & affiliations,
                              TMappingOldMwm * mapping = nullptr);

void LoadCountryFile2CountryInfo(std::string const & jsonBuffer,
                                 map<std::string, CountryInfo> & id2info, bool & isSingleMwm);
}  // namespace storage
