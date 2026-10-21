#include "map/gps_track_storage.hpp"

#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <cstring>  // for memcpy
#include <filesystem>
#include <fstream>

namespace
{

// Current file format version
uint32_t constexpr kCurrentVersion = 1;

// Header size in bytes, header consists of uint32_t 'version' only
uint32_t constexpr kHeaderSize = sizeof(uint32_t);

// TODO
// Now GpsInfo written as plain values, but values can be compressed.

// Size of point in bytes to write in file of read from file
size_t constexpr kPointSize = 8 * sizeof(double) + sizeof(uint8_t);

// Writes value in memory in LittleEndian
template <typename T>
void MemWrite(void * ptr, T value)
{
  value = SwapIfBigEndianMacroBased(value);
  memcpy(ptr, &value, sizeof(T));
}

// Read value from memory, which is LittleEndian in memory
template <typename T>
T MemRead(void const * ptr)
{
  T value;
  memcpy(&value, ptr, sizeof(T));
  return SwapIfBigEndianMacroBased(value);
}

void Pack(char * p, location::GpsInfo const & info)
{
  MemWrite<double>(p + 0 * sizeof(double), info.m_timestamp);
  MemWrite<double>(p + 1 * sizeof(double), info.m_latitude);
  MemWrite<double>(p + 2 * sizeof(double), info.m_longitude);
  MemWrite<double>(p + 3 * sizeof(double), info.m_altitude);
  MemWrite<double>(p + 4 * sizeof(double), info.m_speed);
  MemWrite<double>(p + 5 * sizeof(double), info.m_bearing);
  MemWrite<double>(p + 6 * sizeof(double), info.m_horizontalAccuracy);
  MemWrite<double>(p + 7 * sizeof(double), info.m_verticalAccuracy);
  ASSERT_LESS_OR_EQUAL(static_cast<int>(info.m_source), 255, ());
  uint8_t const source = static_cast<uint8_t>(info.m_source);
  MemWrite<uint8_t>(p + 8 * sizeof(double), source);
}

void Unpack(char const * p, location::GpsInfo & info)
{
  info.m_timestamp = MemRead<double>(p + 0 * sizeof(double));
  info.m_latitude = MemRead<double>(p + 1 * sizeof(double));
  info.m_longitude = MemRead<double>(p + 2 * sizeof(double));
  info.m_altitude = MemRead<double>(p + 3 * sizeof(double));
  info.m_speed = MemRead<double>(p + 4 * sizeof(double));
  info.m_bearing = MemRead<double>(p + 5 * sizeof(double));
  info.m_horizontalAccuracy = MemRead<double>(p + 6 * sizeof(double));
  info.m_verticalAccuracy = MemRead<double>(p + 7 * sizeof(double));
  uint8_t const source = MemRead<uint8_t>(p + 8 * sizeof(double));
  info.m_source = static_cast<location::TLocationSource>(source);
}

inline size_t GetItemOffset(size_t itemIndex)
{
  return kHeaderSize + itemIndex * kPointSize;
}

}  // namespace

GpsTrackStorage::GpsTrackStorage(std::string const & filePath) : m_filePath(filePath) {}

std::vector<GpsTrackStorage::TItem> GpsTrackStorage::Load()
{
  m_itemCount = 0;
  std::error_code error;
  auto const fileSize = std::filesystem::file_size(m_filePath, error);
  if (error)
  {
    if (error != std::errc::no_such_file_or_directory)
    {
      LOG(LERROR, ("Cannot read GPS track backup:", m_filePath, error.message()));
      Clear();
    }
    return {};
  }
  std::ifstream stream(m_filePath, std::ios::binary);
  auto const discardBackup = [this, &stream]
  {
    stream.close();
    Clear();
    return std::vector<TItem>{};
  };

  uint32_t version = 0;
  stream.read(reinterpret_cast<char *>(&version), kHeaderSize);
  if (!stream)
  {
    LOG(LERROR, ("Cannot read GPS track backup header:", m_filePath));
    return discardBackup();
  }
  version = SwapIfBigEndianMacroBased(version);
  if (version != kCurrentVersion)
  {
    LOG(LERROR, ("Unsupported GPS track backup version:", version, m_filePath));
    return discardBackup();
  }

  // An interrupted batch can leave an incomplete final point.
  if ((fileSize - kHeaderSize) % kPointSize != 0)
    LOG(LERROR, ("Incomplete final GPS track backup point:", m_filePath));

  std::vector<TItem> items((fileSize - kHeaderSize) / kPointSize);
  std::vector<char> buffer(items.size() * kPointSize);
  if (!buffer.empty())
  {
    stream.read(buffer.data(), buffer.size());
    if (!stream)
    {
      LOG(LERROR, ("Cannot read GPS track backup points:", m_filePath));
      return discardBackup();
    }

    for (size_t i = 0; i < items.size(); ++i)
      Unpack(buffer.data() + i * kPointSize, items[i]);
  }

  m_itemCount = items.size();
  return items;
}

bool GpsTrackStorage::Append(std::vector<TItem> const & items)
{
  if (items.empty())
    return true;

  std::fstream stream(m_filePath, std::ios::in | std::ios::out | std::ios::binary |
                                      (m_itemCount == 0 ? std::ios::trunc : std::ios::openmode{}));
  if (!stream)
  {
    LOG(LERROR, ("Cannot open GPS track backup for writing:", m_filePath));
    return false;
  }

  if (m_itemCount == 0)
  {
    auto const version = SwapIfBigEndianMacroBased(kCurrentVersion);
    stream.write(reinterpret_cast<char const *>(&version), kHeaderSize);
  }
  stream.seekp(GetItemOffset(m_itemCount));

  std::vector<char> buffer(items.size() * kPointSize);
  for (size_t i = 0; i < items.size(); ++i)
    Pack(buffer.data() + i * kPointSize, items[i]);
  stream.write(buffer.data(), buffer.size());
  stream.close();
  if (!stream)
  {
    LOG(LERROR, ("Cannot write GPS track backup:", m_filePath));
    return false;
  }

  m_itemCount += items.size();
  return true;
}

void GpsTrackStorage::Clear()
{
  m_itemCount = 0;
  std::error_code error;
  std::filesystem::remove(m_filePath, error);
  if (error)
    LOG(LERROR, ("Cannot delete GPS track backup:", m_filePath, error.message()));
}
