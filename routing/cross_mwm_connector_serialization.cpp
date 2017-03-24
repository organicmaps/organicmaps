#include "routing/cross_mwm_connector_serialization.hpp"

#include "base/bits.hpp"

#include <algorithm>

using namespace std;

namespace routing
{
// static
uint32_t constexpr CrossMwmConnectorSerializer::kLastVersion;

// static
uint32_t CrossMwmConnectorSerializer::CalcBitsPerOsmId(std::vector<Transition> const & transitions)
{
  uint64_t maxOsmId = 0;
  for (Transition const & transition : transitions)
    maxOsmId = std::max(maxOsmId, transition.GetOsmId());

  return bits::NumUsedBits(maxOsmId);
}

// static
void CrossMwmConnectorSerializer::WriteTransitions(vector<Transition> const & transitions,
                                                   serial::CodingParams const & codingParams,
                                                   uint32_t bitsPerOsmId, uint8_t bitsPerMask,
                                                   vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> memWriter(buffer);

  for (Transition const & transition : transitions)
    transition.Serialize(codingParams, bitsPerOsmId, bitsPerMask, memWriter);
}

// static
void CrossMwmConnectorSerializer::WriteWeights(vector<Weight> const & weights,
                                               vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> memWriter(buffer);
  BitWriter<MemWriter<vector<uint8_t>>> writer(memWriter);

  CrossMwmConnector::Weight prevWeight = 1;
  for (auto const weight : weights)
  {
    if (weight == CrossMwmConnector::kNoRoute)
    {
      writer.Write(kNoRouteBit, 1);
      continue;
    }

    writer.Write(kRouteBit, 1);
    auto const storedWeight = (weight + kGranularity - 1) / kGranularity;
    WriteDelta(writer, EncodeZigZagDelta(prevWeight, storedWeight) + 1);
    prevWeight = storedWeight;
  }
}
}  // namespace routing
