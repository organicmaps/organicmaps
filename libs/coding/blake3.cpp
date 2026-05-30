#include "coding/blake3.hpp"

#include "coding/base64.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace coding
{
Blake3::Blake3()
{
  blake3_hasher_init(&m_hasher);
}

void Blake3::Update(void const * data, size_t size)
{
  blake3_hasher_update(&m_hasher, data, size);
}

Blake3::Hash Blake3::Finalize() const
{
  Hash result;
  blake3_hasher_finalize(&m_hasher, result.data(), result.size());
  return result;
}

std::string Blake3::FinalizeToBase64(size_t numBytes) const
{
  ASSERT_LESS_OR_EQUAL(numBytes, kHashSizeInBytes, ());
  uint8_t out[kHashSizeInBytes];
  blake3_hasher_finalize(&m_hasher, out, numBytes);
  return base64::Encode(std::string_view(reinterpret_cast<char const *>(out), numBytes));
}

// static
Blake3::Hash Blake3::Calculate(std::string const & filePath)
{
  try
  {
    base::FileData file(filePath, base::FileData::Op::READ);
    uint64_t const fileSize = file.Size();

    Blake3 hasher;
    uint64_t currSize = 0;
    uint32_t constexpr kFileBufferSize = 64 * 1024;
    unsigned char buffer[kFileBufferSize];
    while (currSize < fileSize)
    {
      auto const toRead = std::min(kFileBufferSize, static_cast<uint32_t>(fileSize - currSize));
      file.Read(currSize, buffer, toRead);
      hasher.Update(buffer, toRead);
      currSize += toRead;
    }
    return hasher.Finalize();
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LERROR, ("Error reading file:", filePath, ex.what()));
  }
  return {};
}

// static
std::string Blake3::CalculateBase64(std::string const & filePath, size_t numBytes)
{
  ASSERT_LESS_OR_EQUAL(numBytes, kHashSizeInBytes, ());
  auto const hash = Calculate(filePath);
  return base64::Encode(std::string_view(reinterpret_cast<char const *>(hash.data()), numBytes));
}

// static
Blake3::Hash Blake3::CalculateForString(std::string_view str)
{
  Blake3 hasher;
  hasher.Update(str.data(), str.size());
  return hasher.Finalize();
}
}  // namespace coding
