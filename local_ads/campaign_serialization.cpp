#include "local_ads/campaign_serialization.hpp"

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <climits>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace
{
using namespace local_ads;

DECLARE_EXCEPTION(UnknownVersion, RootException);

auto const kHalfByteShift = CHAR_BIT / 2;
auto const kHalfByteMaxValue = 15;
auto const kLowerMask = 0x0F;
auto const kUpperMask = 0xF0;
auto const kMinZoomLevel = 10;
auto const kMaxZoomLevel = 17;
auto const kMaxPriority = 7;

template <typename Integral, typename Source,
          std::enable_if_t<std::is_integral<Integral>::value, void *> = nullptr>
std::vector<Integral> ReadVarUintArray(Source & s, size_t chunksNumber)
{
  std::vector<Integral> result;
  for (size_t i = 0; i < chunksNumber; ++i)
    result.emplace_back(static_cast<Integral>(ReadVarUint<uint64_t>(s)));

  return result;
}

template <typename Integral, typename Source>
std::vector<Integral> ReadArray(Source & s, size_t chunksNumber)
{
  std::vector<Integral> result;
  for (size_t i = 0; i < chunksNumber; ++i)
  {
    result.emplace_back(ReadPrimitiveFromSource<Integral>(s));
  }

  return result;
}

std::vector<uint8_t> SerializeV1(std::vector<Campaign> const & campaigns)
{
  std::vector<uint8_t> buff;
  PushBackByteSink<decltype(buff)> dst(buff);
  WriteToSink(dst, Version::V1);
  WriteToSink(dst, campaigns.size());
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_featureId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_iconId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_daysBeforeExpired);

  return buff;
}

std::vector<Campaign> DeserializeV1(std::vector<uint8_t> const & bytes)
{
  ReaderSource<MemReaderWithExceptions> src({bytes.data(), bytes.size()});

  CHECK_EQUAL(ReadPrimitiveFromSource<Version>(src), Version::V1, ());
  auto const chunksNumber = ReadPrimitiveFromSource<uint64_t>(src);

  auto const featureIds = ReadVarUintArray<uint32_t>(src, chunksNumber);
  auto const icons = ReadVarUintArray<uint16_t>(src, chunksNumber);
  auto const expirations = ReadVarUintArray<uint8_t>(src, chunksNumber);

  std::vector<Campaign> campaigns;
  campaigns.reserve(chunksNumber);
  for (size_t i = 0; i < chunksNumber; ++i)
  {
    campaigns.emplace_back(featureIds[i], icons[i], expirations[i]);
  }
  return campaigns;
}

uint8_t ZoomIndex(uint8_t zoomValue) { return zoomValue - kMinZoomLevel; }

uint8_t ZoomValue(uint8_t zoomIndex) { return zoomIndex + kMinZoomLevel; }

uint8_t PackZoomAndPriority(uint8_t minZoomLevel, uint8_t priority)
{
  UNUSED_VALUE(kMaxZoomLevel);
  UNUSED_VALUE(kMaxPriority);
  UNUSED_VALUE(kHalfByteMaxValue);

  ASSERT_GREATER_OR_EQUAL(minZoomLevel, kMinZoomLevel, ("Unsupported zoom level"));
  ASSERT_LESS_OR_EQUAL(minZoomLevel, kMaxZoomLevel, ("Unsupported zoom level"));
  ASSERT_LESS_OR_EQUAL(priority, kMaxPriority, ("Unsupported priority value"));

  auto const zoomIndex = ZoomIndex(minZoomLevel);
  ASSERT_LESS_OR_EQUAL(zoomIndex, kHalfByteMaxValue, ());
  ASSERT_LESS_OR_EQUAL(priority, kHalfByteMaxValue, ());

  // Pack zoom and priority into single byte.
  return (zoomIndex & kLowerMask) | ((priority << kHalfByteShift) & kUpperMask);
}

uint8_t UnpackZoom(uint8_t src)
{
  return ZoomValue(src & kLowerMask);
}

uint8_t UnpackPriority(uint8_t src)
{
  return (src >> kHalfByteShift) & kLowerMask;
}

std::vector<uint8_t> SerializeV2(std::vector<Campaign> const & campaigns)
{
  std::vector<uint8_t> buff;
  PushBackByteSink<decltype(buff)> dst(buff);
  WriteToSink(dst, Version::V2);
  WriteToSink(dst, campaigns.size());
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_featureId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_iconId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_daysBeforeExpired);
  for (auto const & c : campaigns)
    WriteToSink(dst, PackZoomAndPriority(c.m_minZoomLevel, c.m_priority));

  return buff;
}

std::vector<Campaign> DeserializeV2(std::vector<uint8_t> const & bytes)
{
  ReaderSource<MemReaderWithExceptions> src({bytes.data(), bytes.size()});

  CHECK_EQUAL(ReadPrimitiveFromSource<Version>(src), Version::V2, ());
  auto const chunksNumber = ReadPrimitiveFromSource<uint64_t>(src);

  auto const featureIds = ReadVarUintArray<uint32_t>(src, chunksNumber);
  auto const icons = ReadVarUintArray<uint16_t>(src, chunksNumber);
  auto const expirations = ReadVarUintArray<uint8_t>(src, chunksNumber);

  auto const zoomAndPriority = ReadArray<uint8_t>(src, chunksNumber);

  std::vector<Campaign> campaigns;
  campaigns.reserve(chunksNumber);
  for (size_t i = 0; i < chunksNumber; ++i)
  {
    campaigns.emplace_back(featureIds[i], icons[i], expirations[i],
                           UnpackZoom(zoomAndPriority[i]),
                           UnpackPriority(zoomAndPriority[i]));

    ASSERT_GREATER_OR_EQUAL(campaigns.back().m_minZoomLevel, kMinZoomLevel,
                            ("Unsupported zoom level"));
    ASSERT_LESS_OR_EQUAL(campaigns.back().m_minZoomLevel, kMaxZoomLevel,
                         ("Unsupported zoom level"));
    ASSERT_LESS_OR_EQUAL(campaigns.back().m_priority, kMaxPriority, ("Unsupported priority value"));
  }
  return campaigns;
}
}  // namespace

namespace local_ads
{
std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns, Version const version)
{
  try
  {
    switch (version)
    {
    case Version::V1: return SerializeV1(campaigns);
    case Version::V2: return SerializeV2(campaigns);
    default: MYTHROW(UnknownVersion, (version));
    }
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Cannot to serialize campaigns", e.what(), e.Msg()));
  }

  return {};
}

std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns)
{
  return Serialize(campaigns, Version::Latest);
}

std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes)
{
  try
  {
    ReaderSource<MemReaderWithExceptions> src({bytes.data(), bytes.size()});

    auto const version = ReadPrimitiveFromSource<Version>(src);

    switch (version)
    {
    case Version::V1: return DeserializeV1(bytes);
    case Version::V2: return DeserializeV2(bytes);
    default: MYTHROW(UnknownVersion, (version));
    }
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Cannot to deserialize received data", e.what(), e.Msg()));
  }
  catch (std::bad_alloc const & e)
  {
    LOG(LERROR, ("Cannot to allocate memory for local ads campaigns", e.what()));
  }

  return {};
}

std::string DebugPrint(local_ads::Version version)
{
  using local_ads::Version;

  switch (version)
  {
  case Version::Unknown: return "Unknown";
  case Version::V1: return "Version 1";
  case Version::V2: return "Version 2";
  default: ASSERT(false, ("Unknown version"));
  }

  return {};
}
}  // namespace local_ads
