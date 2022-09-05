#pragma once

#include "platform/mwm_version.hpp"

#include <string>

namespace version
{
// This is a wrapper around mwm's version.  Allows users to get
// information about versions of some data structures in mwm.
class MwmTraits
{
public:
  enum class SearchIndexFormat
  {
    // A list of features with their ranks and centers
    // is stored behind every node of the search trie.
    // This format corresponds to ValueList<FeatureWithRankAndCenter>.
    FeaturesWithRankAndCenter,

    // A compressed bit vector of feature indices is
    // stored behind every node of the search trie.
    // This format corresponds to ValueList<Uint64IndexValue>.
    CompressedBitVector,

    // A compressed bit vector of feature indices is
    // stored behind every node of the search trie.
    // This format corresponds to ValueList<Uint64IndexValue>.
    // Section has header.
    CompressedBitVectorWithHeader,
  };

  enum class HouseToStreetTableFormat
  {
    // Versioning is independent of MwmTraits: section format depends on the section header.
    HouseToStreetTableWithHeader,

    // The format of relation is unknown. Most likely, an error has occured.
    Unknown
  };

  enum class CentersTableFormat
  {
    // Centers table encoded without any header. Coding params from mwm header are used.
    PlainEliasFanoMap,

    // Centers table has its own header with version and coding params.
    EliasFanoMapWithHeader,
  };

  explicit MwmTraits(MwmVersion const & version);

  SearchIndexFormat GetSearchIndexFormat() const;

  HouseToStreetTableFormat GetHouseToStreetTableFormat() const;

  CentersTableFormat GetCentersTableFormat() const;

  bool HasIsolines() const;

private:
  Format GetFormat() const { return m_version.GetFormat(); }
  uint32_t GetVersion() const { return m_version.GetVersion(); }

  MwmVersion m_version;
};

std::string DebugPrint(MwmTraits::SearchIndexFormat format);
std::string DebugPrint(MwmTraits::HouseToStreetTableFormat format);
std::string DebugPrint(MwmTraits::CentersTableFormat format);
}  // namespace version
