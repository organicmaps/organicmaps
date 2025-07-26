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
    m_notEnoughScore += s.m_notEnoughScore;
    m_wrongOffsets += s.m_wrongOffsets;
    m_zeroDistToNextPointCount += s.m_zeroDistToNextPointCount;
  }

  void Report() const
  {
    LOG(LINFO, ("Total routes handled:", m_routesHandled));
    LOG(LINFO, ("Failed:", m_routesFailed));
    LOG(LINFO, ("No candidate lines:", m_noCandidateFound));
    LOG(LINFO, ("Wrong distance to next point:", m_zeroDistToNextPointCount));
    LOG(LINFO, ("Not enough score for shortest path:", m_notEnoughScore));
    LOG(LINFO, ("Wrong offsets:", m_wrongOffsets));
    LOG(LINFO, ("No shortest path:", m_noShortestPathFound));
  }

  uint32_t m_routesHandled = 0;
  uint32_t m_routesFailed = 0;
  uint32_t m_noCandidateFound = 0;
  uint32_t m_noShortestPathFound = 0;
  uint32_t m_notEnoughScore = 0;
  uint32_t m_wrongOffsets = 0;
  // Number of zeroed distance-to-next point values in the input.
  uint32_t m_zeroDistToNextPointCount = 0;
};
}  // namespace v2
}  // namespace openlr
