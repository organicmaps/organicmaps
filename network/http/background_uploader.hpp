#pragma once

#include "network/http/payload.hpp"

namespace om::network::http
{
class BackgroundUploader
{
public:
  explicit BackgroundUploader(Payload const & payload) : m_payload(payload) {}
  Payload const & GetPayload() const { return m_payload; }

  // TODO add platform-specific implementation
  void Upload() const;

private:
  Payload m_payload;
};
}  // namespace om::network::http
