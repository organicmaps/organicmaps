#pragma once

#include "platform/http_payload.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace platform
{
class HttpUploader
{
public:
  struct Result
  {
    int32_t m_httpCode = 0;
    std::string m_description;
  };

  HttpUploader() = delete;
  explicit HttpUploader(HttpPayload const & payload) : m_payload(payload) {}
  HttpPayload const & GetPayload() const { return m_payload; }
  Result Upload() const;

private:
  HttpPayload const m_payload;
};
}  // namespace platform
