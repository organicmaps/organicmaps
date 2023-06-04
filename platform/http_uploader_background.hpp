#pragma once

#include "platform/http_payload.hpp"

namespace platform
{
class HttpUploaderBackground
{
public:
  HttpUploaderBackground() = delete;
  explicit HttpUploaderBackground(HttpPayload const & payload) : m_payload(payload) {}
  HttpPayload const & GetPayload() const { return m_payload; }

  // TODO add platform-specific implementation
  void Upload() const;

private:
  HttpPayload m_payload;
};
}  // namespace platform
