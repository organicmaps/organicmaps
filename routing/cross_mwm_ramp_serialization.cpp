#include "routing/cross_mwm_ramp_serialization.hpp"

using namespace std;

namespace routing
{
// static
uint32_t constexpr CrossMwmRampSerializer::kLastVersion;

// static
void CrossMwmRampSerializer::WriteTransitions(vector<Transition> const & transitions,
                                              serial::CodingParams const & codingParams,
                                              uint8_t bitsPerMask, vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> memWriter(buffer);

  for (Transition const & transition : transitions)
    transition.Serialize(codingParams, bitsPerMask, memWriter);
}

// static
void CrossMwmRampSerializer::WriteWeights(vector<CrossMwmRamp::Weight> const & weights,
                                          vector<uint8_t> & buffer)
{
  MemWriter<vector<uint8_t>> memWriter(buffer);

  for (auto weight : weights)
    WriteToSink(memWriter, weight);
}
}  // namespace routing
