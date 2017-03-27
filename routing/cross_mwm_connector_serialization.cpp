#include "routing/cross_mwm_connector_serialization.hpp"

using namespace std;

namespace routing
{
// static
uint32_t constexpr CrossMwmConnectorSerializer::kLastVersion;

// static
void CrossMwmConnectorSerializer::WriteTransitions(vector<Transition> const & transitions,
                                                   serial::CodingParams const & codingParams,
                                                   uint8_t bitsPerMask, vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> memWriter(buffer);

  for (Transition const & transition : transitions)
    transition.Serialize(codingParams, bitsPerMask, memWriter);
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
