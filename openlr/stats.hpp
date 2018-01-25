#pragma once

#include "openlr/cache_line_size.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <cstdint>

namespace openlr
{
namespace v2
{
struct alignas(kCacheLineSize) Stats
{
  void Add(Stats const & s)
  {
    m_routesHandled += s.m_routesHandled;
    m_routesFailed += s.m_routesFailed;
    m_noCandidateFound += s.m_noCandidateFound;
    m_noShortestPathFound += s.m_noShortestPathFound;
    m_wrongOffsets += s.m_wrongOffsets;
    m_dnpIsZero += s.m_dnpIsZero;
  }

  void Report() const
  {
    LOG(LINFO, ("Total routes handled:", m_routesHandled));
    LOG(LINFO, ("Failed:", m_routesFailed));
    LOG(LINFO, ("No candidate lines:", m_noCandidateFound));
    LOG(LINFO, ("Wrong distance to next point:", m_dnpIsZero));
    LOG(LINFO, ("Wrong offsets:", m_wrongOffsets));
    LOG(LINFO, ("No shortest path:", m_noShortestPathFound));
  }

  uint32_t m_routesHandled = 0;
  uint32_t m_routesFailed = 0;
  uint32_t m_noCandidateFound = 0;
  uint32_t m_noShortestPathFound = 0;
  uint32_t m_wrongOffsets = 0;
  // Number of zeroed distance-to-next point values in the input.
  uint32_t m_dnpIsZero = 0;
};
}  // namespace V2
}  // namespace openlr
