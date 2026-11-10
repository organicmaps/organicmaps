#include "map/bookmarks_sync/content_hash.hpp"

#include "coding/blake3.hpp"
#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"

#include <algorithm>

#include <boost/hash2/md5.hpp>

namespace bookmarks_sync
{
std::string HashFile(std::string const & path, HashAlgorithm algorithm)
{
  switch (algorithm)
  {
  case HashAlgorithm::Blake3: return coding::Blake3::CalculateBase64(path);
  case HashAlgorithm::Md5:
  {
    base::FileData file(path, base::FileData::Op::READ);
    uint64_t const size = file.Size();
    boost::hash2::md5_128 md5;
    uint64_t offset = 0;
    uint32_t constexpr kBufferSize = 64 * 1024;
    unsigned char buffer[kBufferSize];
    while (offset < size)
    {
      auto const toRead = static_cast<size_t>(std::min<uint64_t>(kBufferSize, size - offset));
      file.Read(offset, buffer, toRead);
      md5.update(buffer, toRead);
      offset += toRead;
    }
    return boost::hash2::to_string(md5.result());
  }
  }
  UNREACHABLE();
}
}  // namespace bookmarks_sync
