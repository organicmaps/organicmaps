#pragma once

#include "indexer/cycling_infrastructure.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace cycling
{
inline constexpr char kSectionTag[] = "cycling_infra";
inline constexpr uint32_t kSectionVersion = 1;
inline constexpr uint64_t kSectionHeaderSize = 8;
inline constexpr uint64_t kIndexEntrySize = 16;
inline constexpr uint32_t kMaxPayloadSize = 65536;

// Sparse, immutable feature-ID index. Readers use positional I/O, so no shared
// mutable seek position or region-sized in-memory table is needed for rendering.
template <typename Sink>
void WriteSection(Sink & sink, std::vector<std::pair<uint32_t, std::string>> const & records)
{
  CHECK_LESS_OR_EQUAL(records.size(), std::numeric_limits<uint32_t>::max(), ());
  WriteToSink(sink, kSectionVersion);
  WriteToSink(sink, static_cast<uint32_t>(records.size()));
  uint64_t offset = kSectionHeaderSize + records.size() * kIndexEntrySize;
  for (size_t i = 0; i < records.size(); ++i)
  {
    auto const & [id, payload] = records[i];
    CHECK(i == 0 || records[i - 1].first < id, (id));
    CHECK_LESS_OR_EQUAL(payload.size(), kMaxPayloadSize, (id));
    CHECK(Decode(payload), (id));
    WriteToSink(sink, id);
    WriteToSink(sink, offset);
    WriteToSink(sink, static_cast<uint32_t>(payload.size()));
    offset += payload.size();
  }
  for (auto const & [id, payload] : records)
    sink.Write(payload.data(), payload.size());
}

template <typename Reader>
std::string ReadSection(Reader const & reader, uint32_t featureId)
{
  CHECK_GREATER_OR_EQUAL(reader.Size(), kSectionHeaderSize, ());
  ReaderSource source(reader);
  if (ReadPrimitiveFromSource<uint32_t>(source) != kSectionVersion)
    return {};
  uint32_t const count = ReadPrimitiveFromSource<uint32_t>(source);
  CHECK_LESS_OR_EQUAL(count, (reader.Size() - kSectionHeaderSize) / kIndexEntrySize, ());
  uint64_t const dataOffset = kSectionHeaderSize + uint64_t(count) * kIndexEntrySize;
  uint32_t low = 0, high = count;
  while (low < high)
  {
    uint32_t const mid = low + (high - low) / 2;
    ReaderSource row(reader);
    row.Skip(kSectionHeaderSize + uint64_t(mid) * kIndexEntrySize);
    uint32_t const id = ReadPrimitiveFromSource<uint32_t>(row);
    if (id < featureId)
      low = mid + 1;
    else if (id > featureId)
      high = mid;
    else
    {
      uint64_t const offset = ReadPrimitiveFromSource<uint64_t>(row);
      uint32_t const length = ReadPrimitiveFromSource<uint32_t>(row);
      CHECK_GREATER_OR_EQUAL(offset, dataOffset, ());
      CHECK_LESS_OR_EQUAL(offset, reader.Size(), ());
      CHECK_LESS_OR_EQUAL(length, kMaxPayloadSize, ());
      CHECK_LESS_OR_EQUAL(length, reader.Size() - offset, ());
      std::string payload(length, '\0');
      reader.Read(offset, payload.data(), length);
      // Server-produced data is trusted in Release. Catch writer mistakes in
      // Debug, without decoding/allocating the side vectors twice per lookup.
      ASSERT(Decode(payload), (featureId));
      return payload;
    }
  }
  return {};
}
}  // namespace cycling
