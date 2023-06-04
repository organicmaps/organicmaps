#include "routing/cross_mwm_connector.hpp"

namespace routing
{
namespace connector
{
std::string DebugPrint(WeightsLoadState state)
{
  switch (state)
  {
  case WeightsLoadState::Unknown: return "Unknown";
  case WeightsLoadState::ReadyToLoad: return "ReadyToLoad";
  case WeightsLoadState::NotExists: return "NotExists";
  case WeightsLoadState::Loaded: return "Loaded";
  }
  UNREACHABLE();
}
}  // namespace connector
}  // namespace routing
