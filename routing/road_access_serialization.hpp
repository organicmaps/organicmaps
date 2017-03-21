#pragma once

#include "routing/road_access.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace routing
{
class RoadAccessSerializer final
{
public:
  RoadAccessSerializer() = delete;

  template <class Sink>
  static void Serialize(Sink & sink, RoadAccess const & roadAccess)
  {
    uint32_t const header = kLatestVersion;
    WriteToSink(sink, header);

    auto const & privateRoads = roadAccess.GetPrivateRoads();
    ASSERT(std::is_sorted(privateRoads.begin(), privateRoads.end()), ());
    WriteToSink(sink, base::checked_cast<uint32_t>(privateRoads.size()));
    if (!privateRoads.empty())
      WriteVarUint(sink, privateRoads[0]);
    for (size_t i = 1; i < privateRoads.size(); ++i)
    {
      uint32_t const delta = privateRoads[i] - privateRoads[i - 1];
      WriteVarUint(sink, delta);
    }
  }

  template <class Source>
  static void Deserialize(Source & src, RoadAccess & roadAccess)
  {
    uint32_t const header = ReadPrimitiveFromSource<uint32_t>(src);
    CHECK_EQUAL(header, kLatestVersion, ());
    size_t numPrivateRoads = base::checked_cast<size_t>(ReadPrimitiveFromSource<uint32_t>(src));
    std::vector<uint32_t> privateRoads(numPrivateRoads);
    if (numPrivateRoads > 0)
      privateRoads[0] = ReadVarUint<uint32_t>(src);
    for (size_t i = 1; i < numPrivateRoads; ++i)
    {
      uint32_t delta = ReadVarUint<uint32_t>(src);
      privateRoads[i] = privateRoads[i - 1] + delta;
    }
    roadAccess.SetPrivateRoads(move(privateRoads));
  }

private:
  uint32_t static const kLatestVersion;
};
}  // namespace routing
