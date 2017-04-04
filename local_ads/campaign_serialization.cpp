#include "local_ads/campaign_serialization.hpp"

#include "coding/varint.hpp"
#include "coding/byte_stream.hpp"

#include "base/stl_add.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace
{
enum class Version
{
  unknown = -1,
  v1 = 0,  // March 2017 (Store feature ids and icon ids as varints,
           // use one byte for days before expiration.)
  latest = v1
};

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
}  // namespace

namespace local_ads
{
std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns)
{
  std::vector<uint8_t> buff;
  PushBackByteSink<decltype(buff)> dst(buff);
  Write(dst, Version::latest);
  Write(dst, campaigns.size());
  for (auto const & c : campaigns)
     WriteVarUint(dst, c.m_featureId);
  for (auto const & c : campaigns)
    WriteVarUint(dst, c.m_iconId);

  for (auto const & c : campaigns)
    Write(dst, c.m_daysBeforeExpired);
  return buff;
}

std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes)
{
  ArrayByteSource src(bytes.data());
  auto const version = Read<Version>(src);
  static_cast<void>(version);  // No version dispatching for now.
  auto const chunksNumber = Read<size_t>(src);

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
    campaigns.emplace_back(
        featureIds[i],
        icons[i],
        expirations[i],
        false /* priorityBit */
    );
  }
  return campaigns;
}
}  // namespace local_ads
