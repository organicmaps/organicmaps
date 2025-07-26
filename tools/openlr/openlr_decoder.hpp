#pragma once

#include "openlr/stats.hpp"

#include "indexer/data_source.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace openlr
{
struct LinearSegment;
struct DecodedPath;

DECLARE_EXCEPTION(DecoderError, RootException);

class Graph;
class RoadInfoGetter;

class OpenLRDecoder
{
public:
  using CountryParentNameGetter = std::function<std::string(std::string const &)>;

  class SegmentsFilter
  {
  public:
    SegmentsFilter(std::string const & idsPath, bool const multipointsOnly);

    bool Matches(LinearSegment const & segment) const;

  private:
    std::unordered_set<uint32_t> m_ids;
    bool m_idsSet;
    bool const m_multipointsOnly;
  };

  OpenLRDecoder(std::vector<FrozenDataSource> & dataSources, CountryParentNameGetter const & countryParentNameGetter);

  // Maps partner segments to mwm paths. |segments| should be sorted by partner id.
  void DecodeV2(std::vector<LinearSegment> const & segments, uint32_t const numThreads,
                std::vector<DecodedPath> & paths);

  void DecodeV3(std::vector<LinearSegment> const & segments, uint32_t numThreads, std::vector<DecodedPath> & paths);

private:
  template <typename Decoder, typename Stats>
  void Decode(std::vector<LinearSegment> const & segments, uint32_t const numThreads, std::vector<DecodedPath> & paths);

  std::vector<FrozenDataSource> & m_dataSources;
  CountryParentNameGetter m_countryParentNameGetter;
};
}  // namespace openlr
