#include "platform/http_payload.hpp"

namespace platform
{
HttpPayload::HttpPayload() : m_method("POST"), m_fileKey("file"), m_needClientAuth(false) {}
}  // namespace platform
