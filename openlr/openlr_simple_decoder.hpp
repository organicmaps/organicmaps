#pragma once

#include "base/exception.hpp"

#include <routing/road_graph.hpp>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

class Index;

namespace openlr
{
struct LinearSegment;
struct DecodedPath;

DECLARE_EXCEPTION(DecoderError, RootException);

class OpenLRSimpleDecoder
{
public:
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

  OpenLRSimpleDecoder(std::vector<Index> const & indexes);

  // Maps partner segments to mwm paths. |segments| should be sorted by partner id.
  void Decode(std::vector<LinearSegment> const & segments, uint32_t const numThreads,
              std::vector<DecodedPath> & paths);

private:
  std::vector<Index> const & m_indexes;
};
}  // namespace openlr
