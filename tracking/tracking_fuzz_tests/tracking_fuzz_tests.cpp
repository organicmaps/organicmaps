#include "tracking/protocol.hpp"

#include "base/logging.hpp"

#include <iostream>
#include <vector>

using namespace tracking;

extern "C" int LLVMFuzzerTestOneInput(uint8_t const * data, size_t size)
{
  base::ScopedLogLevelChanger scopedLogLevelChanger(base::LCRITICAL);

  std::vector const dataVec(data, data + size);
  Protocol::DecodeHeader(dataVec);
  for (auto const type : {Protocol::PacketType::Error, Protocol::PacketType::AuthV0,
                          Protocol::PacketType::DataV0, Protocol::PacketType::DataV1})
  {
    Protocol::DecodeAuthPacket(type, dataVec);
    Protocol::DecodeDataPacket(type, dataVec);
  }
  return 0;
}
