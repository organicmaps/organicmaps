#include "coding/reader_writer_ops.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rw_ops
{
void Reverse(Reader const & src, Writer & dest)
{
  // Read from end, reverse and write directly.

  size_t const bufSz = 1024;
  std::vector<uint8_t> buffer(bufSz);

  uint64_t pos = src.Size();
  while (pos > 0)
  {
    size_t const sz = pos > bufSz ? bufSz : static_cast<size_t>(pos);
    ASSERT_GREATER_OR_EQUAL(pos, sz, ());

    src.Read(pos - sz, &buffer[0], sz);

    std::reverse(buffer.begin(), buffer.begin() + sz);

    dest.Write(&buffer[0], sz);

    pos -= sz;
  }
}
}  // namespace rw_ops
