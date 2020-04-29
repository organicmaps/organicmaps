#pragma once

#include "storage/country_decl.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "platform/country_defines.hpp"

#include "geometry/rect2d.hpp"

#include "defines.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace storage
{
/// This class keeps all the information about a country in country tree (CountryTree).
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
public:
  Country() = default;
  explicit Country(CountryId const & name, CountryId const & parent = kInvalidCountryId)
    : m_name(name), m_parent(parent)
  {
  }

  void SetFile(platform::CountryFile const & file) { m_file = file; }
  void SetSubtreeAttrs(MwmCounter subtreeMwmNumber, MwmSize subtreeMwmSizeBytes)
  {
    m_subtreeMwmNumber = subtreeMwmNumber;
    m_subtreeMwmSizeBytes = subtreeMwmSizeBytes;
  }
  MwmCounter GetSubtreeMwmCounter() const { return m_subtreeMwmNumber; }
  MwmSize GetSubtreeMwmSizeBytes() const { return m_subtreeMwmSizeBytes; }
  CountryId GetParent() const { return m_parent; }

  /// This function valid for current logic - one file for one country (region).
  /// If the logic will be changed, replace GetFile with ForEachFile.
  platform::CountryFile const & GetFile() const { return m_file; }
  CountryId const & Name() const { return m_name; }

private:
  /// Name in the country node tree. In single mwm case it's a country id.
  CountryId m_name;
  /// Country id of parent of m_name in country tree. m_parent == kInvalidCountryId for the root.
  CountryId m_parent;
  /// |m_file| is a CountryFile of mwm with id == |m_name|.
  /// if |m_name| is node id of a group of mwms, |m_file| is empty.
  platform::CountryFile m_file;
  /// The number of descendant mwm files of |m_name|. Only files (leaves in tree) are counted.
  /// If |m_name| is a mwm file name |m_childrenNumber| == 1.
  MwmCounter m_subtreeMwmNumber = 0;
  /// Size of descendant mwm files of |m_name|.
  /// If |m_name| is a mwm file name |m_subtreeMwmSizeBytes| is equal to size of the mwm.
  MwmSize m_subtreeMwmSizeBytes = 0;
};
}  // namespace storage
