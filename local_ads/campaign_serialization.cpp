#include "local_ads/campaign_serialization.hpp"

#include "coding/varint.hpp"
#include "coding/byte_stream.hpp"

#include "base/stl_add.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace
{
using namespace local_ads;

auto const kHalfByteShift = 0x4;
auto const kLowerMask = 0xF;
auto const kUpperMask = 0xF0;
auto const kMinZoomLevel = 10;
auto const kMaxZoomLevel = 17;
auto const kMaxPriority = 7;

template<typename T>
constexpr bool IsEnumOrIntegral()
{
  return std::is_integral<T>::value || std::is_enum<T>::value;
}

template<typename ByteStream, typename T,
         typename std::enable_if<IsEnumOrIntegral<T>(), void>::type * = nullptr>
void Write(ByteStream & s, T t)
{
  s.Write(&t, sizeof(T));
}

template<typename T, typename ByteStream,
         typename std::enable_if<IsEnumOrIntegral<T>(), void>::type * = nullptr>
T Read(ByteStream & s)
{
  T t;
  s.Read(static_cast<void*>(&t), sizeof(t));
  return t;
}

template<typename Integral,
         typename ByteStream,
         typename std::enable_if<std::is_integral<Integral>::value, void*>::type = nullptr>
std::vector<Integral> ReadData(ByteStream & s, size_t chunksNumber)
{
  std::vector<Integral> result;
  result.reserve(chunksNumber);
  auto const streamBegin = s.PtrUC();
  auto const afterLastReadByte = ReadVarUint64Array(
      streamBegin,
      chunksNumber,
      MakeBackInsertFunctor(result)
  );

  ASSERT_EQUAL(result.size(), chunksNumber, ());
  s.Advance(static_cast<decltype(streamBegin)>(afterLastReadByte) - streamBegin);

  return result;
}
std::vector<Campaign> DeserializeV1(std::vector<uint8_t> const & bytes)
{
  ArrayByteSource src(bytes.data());
  CHECK_EQUAL(Read<Version>(src), Version::v1, ());
  auto const chunksNumber = Read<uint64_t>(src);

  auto const featureIds = ReadData<uint32_t>(src, chunksNumber);
  auto const icons = ReadData<uint16_t>(src, chunksNumber);
  auto const expirations = ReadData<uint8_t>(src, chunksNumber);

  CHECK_EQUAL(featureIds.size(), chunksNumber, ());
  CHECK_EQUAL(icons.size(), chunksNumber, ());
  CHECK_EQUAL(expirations.size(), chunksNumber, ());

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

std::vector<Campaign> DeserializeV2(std::vector<uint8_t> const & bytes)
{
  ArrayByteSource src(bytes.data());
  CHECK_EQUAL(Read<Version>(src), Version::v2, ());
  auto const chunksNumber = Read<uint64_t>(src);

  auto const featureIds = ReadData<uint32_t>(src, chunksNumber);
  auto const icons = ReadData<uint16_t>(src, chunksNumber);
  auto const expirations = ReadData<uint8_t>(src, chunksNumber);
  auto const zoomAndPriority = ReadData<uint8_t>(src, chunksNumber);

  CHECK_EQUAL(featureIds.size(), chunksNumber, ());
  CHECK_EQUAL(icons.size(), chunksNumber, ());
  CHECK_EQUAL(expirations.size(), chunksNumber, ());
  CHECK_EQUAL(zoomAndPriority.size(), chunksNumber, ());

  std::vector<Campaign> campaigns;
  campaigns.reserve(chunksNumber);
  for (size_t i = 0; i < chunksNumber; ++i)
  {
    campaigns.emplace_back(featureIds[i], icons[i], expirations[i],
                           ZoomValue(zoomAndPriority[i] & kLowerMask),
                           (zoomAndPriority[i] >> kHalfByteShift) & kLowerMask);

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
  std::vector<uint8_t> buff;
  PushBackByteSink<decltype(buff)> dst(buff);
  Write(dst, version);
  Write(dst, campaigns.size());
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_featureId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_iconId);
  for (auto const & c : campaigns)
    Write(dst, c.m_daysBeforeExpired);
  for (auto const & c : campaigns)
  {
    ASSERT_GREATER_OR_EQUAL(c.m_minZoomLevel, kMinZoomLevel, ("Unsupported zoom level"));
    ASSERT_LESS_OR_EQUAL(c.m_minZoomLevel, kMaxZoomLevel, ("Unsupported zoom level"));
    ASSERT_LESS_OR_EQUAL(c.m_priority, kMaxPriority, ("Unsupported priority value"));

    Write(dst, static_cast<uint8_t>((ZoomIndex(c.m_minZoomLevel) & kLowerMask) |
                                    ((c.m_priority << kHalfByteShift) & kUpperMask)));
  }

  return buff;
}

std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns)
{
  return Serialize(campaigns, Version::latest);
}

std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes)
{
  ArrayByteSource src(bytes.data());
  auto const version = Read<Version>(src);

  switch (version)
  {
  case Version::v1: return DeserializeV1(bytes);
  case Version::v2: return DeserializeV2(bytes);
  default: ASSERT(false, ("Unknown version type"));
  }

  return {};
}

std::string DebugPrint(local_ads::Version version)
{
  using local_ads::Version;

  switch (version)
  {
  case Version::unknown: return "Unknown";
  case Version::v1: return "Version 1";
  case Version::v2: return "Version 2";
  }
}
}  // namespace local_ads
