#pragma once

#include <cstdint>
#include <limits>

namespace routing
{
namespace transit
{
using LineId = uint32_t;
using StopId = uint64_t;
using TransfersId = uint64_t;
using NetworkId = uint32_t;
using FeatureId = uint32_t;

LineId constexpr kLineIdInvalid = std::numeric_limits<LineId>::max();
StopId constexpr kStopIdInvalid = std::numeric_limits<StopId>::max();
TransfersId constexpr kTransfersIdInvalid = std::numeric_limits<TransfersId>::max();
NetworkId constexpr kNetworkIdInvalid = std::numeric_limits<NetworkId>::max();
FeatureId constexpr kFeatureIdInvalid = std::numeric_limits<FeatureId>::max();

}  // namespace transit
}  // namespace routing