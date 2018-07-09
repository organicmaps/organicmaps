#include "coding/sha1.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include "3party/liboauthcpp/src/SHA1.h"
#include "3party/liboauthcpp/src/base64.h"

#include <algorithm>

namespace coding
{
// static
SHA1::Hash SHA1::Calculate(std::string const & filePath)
{
  uint32_t constexpr kFileBufferSize = 8192;
  try
  {
    my::FileData file(filePath, my::FileData::OP_READ);
    uint64_t const fileSize = file.Size();

    CSHA1 sha1;
    uint64_t currSize = 0;
    unsigned char buffer[kFileBufferSize];
    while (currSize < fileSize)
    {
      auto const toRead = std::min(kFileBufferSize, static_cast<uint32_t>(fileSize - currSize));
      file.Read(currSize, buffer, toRead);
      sha1.Update(buffer, toRead);
      currSize += toRead;
    }
    sha1.Final();

    Hash result;
    ASSERT_EQUAL(result.size(), ARRAY_SIZE(sha1.m_digest), ());
    std::copy(std::begin(sha1.m_digest), std::end(sha1.m_digest), std::begin(result));
    return result;
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LERROR, ("Error reading file:", filePath, ex.what()));
  }
  return {};
}

// static
std::string SHA1::CalculateBase64(std::string const & filePath)
{
  auto sha1 = Calculate(filePath);
  return base64_encode(sha1.data(), sha1.size());
}
}  // coding
