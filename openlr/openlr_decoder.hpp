#pragma once

#include "openlr/stats.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

class Index;

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

  OpenLRDecoder(std::vector<Index> const & indexes,
                CountryParentNameGetter const & countryParentNameGetter);

  // Maps partner segments to mwm paths. |segments| should be sorted by partner id.
  void DecodeV1(std::vector<LinearSegment> const & segments, uint32_t const numThreads,
                std::vector<DecodedPath> & paths);

  void DecodeV2(std::vector<LinearSegment> const & segments, uint32_t const /* numThreads */,
                std::vector<DecodedPath> & paths);

private:
  bool DecodeSingleSegment(LinearSegment const & segment, Index const & index, Graph & g,
                           RoadInfoGetter & inforGetter, DecodedPath & path, v2::Stats & stat);

  std::vector<Index> const & m_indexes;
  CountryParentNameGetter m_countryParentNameGetter;
};
}  // namespace openlr
