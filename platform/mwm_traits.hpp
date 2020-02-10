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
    // An array of elements where i-th element is an index of a street
    // in a vector returned by ReverseGeocoder::GetNearbyStreets() for
    // the i-th feature.  Each element normally fits into 3 bits, but
    // there can be exceptions, and these exceptions are stored in a
    // separate table. See ReverseGeocoder and FixedBitsDDVector for
    // details.
    Fixed3BitsDDVector,

    // Elias-Fano based map from feature id to corresponding street feature id.
    EliasFanoMap,

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

  bool HasOffsetsTable() const;

  bool HasCrossMwmSection() const;

  // The new routing section with IndexGraph was added in december 2016.
  // Check whether mwm has routing index section.
  bool HasRoutingIndex() const;

  bool HasCuisineTypes() const;

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
