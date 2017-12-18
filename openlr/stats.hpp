#pragma once

#include <chrono>
#include <cstdint>

namespace openlr
{
namespace v2
{
struct Stats
{
  uint32_t m_routesHandled = 0;
  uint32_t m_routesFailed = 0;
  uint32_t m_noCandidateFound = 0;
  uint32_t m_noShortestPathFound = 0;
  uint32_t m_wrongOffsets = 0;
  // Number of zeroed distance-to-next point values in the input.
  uint32_t m_dnpIsZero = 0;

  std::chrono::milliseconds m_pointCadtidatesTime{0};
  std::chrono::milliseconds m_startLinesTime{0};
  std::chrono::milliseconds m_lineCandidatesTime{0};
  std::chrono::milliseconds m_allStatPathsTime{0};
  std::chrono::milliseconds m_bestCandidatePathsTime{0};
  std::chrono::milliseconds m_shortestPathsTime{0};
};
}  // namespace V2
}  // namespace openlr
