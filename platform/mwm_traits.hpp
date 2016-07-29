#pragma once

#include "platform/mwm_version.hpp"

#include "std/string.hpp"

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
    // This format corresponds to ValueList<FeatureIndexValue>.
    CompressedBitVector,
  };

  enum class HouseToStreetTableFormat
  {
    // An array of elements where i-th element is an index of a street
    // in a vector returned by ReverseGeocoder::GetNearbyStreets() for
    // the i-th feature.  Each element normally fits into 3 bits, but
    // there can be exceptions, and these exceptions are stored in a
    // separate table. See ReverseGeocoder and FixedBitsDDVector for
    // details.
    Fixed3BitsDDVector,

    // The format of relation is unknown. Most likely, an error has occured.
    Unknown
  };

  MwmTraits(version::Format versionFormat);

  SearchIndexFormat GetSearchIndexFormat() const;

  HouseToStreetTableFormat GetHouseToStreetTableFormat() const;

  bool HasOffsetsTable() const;

private:
  version::Format m_versionFormat;
};

string DebugPrint(MwmTraits::SearchIndexFormat format);
string DebugPrint(MwmTraits::HouseToStreetTableFormat format);
}  // namespace version
