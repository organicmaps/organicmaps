#include "routing/cross_mwm_connector.hpp"

namespace routing
{
namespace connector
{
std::string_view DebugPrint(WeightsLoadState state)
{
  switch (state)
  {
    using enum WeightsLoadState;
  case Unknown: return "Unknown";
  case ReadyToLoad: return "ReadyToLoad";
  case NotExists: return "NotExists";
  case Loaded: return "Loaded";
  }
  UNREACHABLE();
}
}  // namespace connector
}  // namespace routing
