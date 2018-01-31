#include "platform/http_uploader.hpp"

#include "base/assert.hpp"

namespace platform
{
void HttpUploader::Upload() const
{
  // Dummy implementation.
  CHECK(m_callback, ());
}
} // namespace platform
