#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "network/http/payload.hpp"

namespace om::network::http
{
class Uploader
{
public:
  struct Result
  {
    int32_t m_httpCode = 0;
    std::string m_description;
  };

  explicit Uploader(Payload const & payload) : m_payload(payload) {}
  Payload const & GetPayload() const { return m_payload; }
  Result Upload() const;

private:
  Payload const m_payload;
};
}  // namespace om::network::http
