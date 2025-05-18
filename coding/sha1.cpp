#include "coding/sha1.hpp"
#include "coding/base64.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <boost/core/bit.hpp>
#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <vector>

namespace coding
{
namespace
{
SHA1::Hash ExtractHash(boost::uuids::detail::sha1 & sha1)
{
  boost::uuids::detail::sha1::digest_type digest;
  sha1.get_digest(digest);
  for (auto & b : digest)
    b = boost::core::byteswap(b);

  SHA1::Hash result;
  static_assert(result.size() == sizeof(digest));
  std::copy_n(reinterpret_cast<uint8_t const *>(digest), sizeof(digest), std::begin(result));
  return result;
}
}

// static
SHA1::Hash SHA1::Calculate(std::string const & filePath)
{
  try
  {
    base::FileData file(filePath, base::FileData::Op::READ);
    uint64_t const fileSize = file.Size();

    boost::uuids::detail::sha1 sha1;

    uint64_t currSize = 0;
    uint32_t constexpr kFileBufferSize = 8192;
    unsigned char buffer[kFileBufferSize];
    while (currSize < fileSize)
    {
      auto const toRead = std::min(kFileBufferSize, static_cast<uint32_t>(fileSize - currSize));
      file.Read(currSize, buffer, toRead);
      sha1.process_bytes(buffer, toRead);
      currSize += toRead;
    }
    return ExtractHash(sha1);
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
  auto const sha1 = Calculate(filePath);
  return base64::Encode(std::string_view(reinterpret_cast<char const *>(sha1.data()), sha1.size()));
}

// static
SHA1::Hash SHA1::CalculateForString(std::string_view str)
{
  boost::uuids::detail::sha1 sha1;
  sha1.process_bytes(str.data(), str.size());
  return ExtractHash(sha1);
}
}  // coding
