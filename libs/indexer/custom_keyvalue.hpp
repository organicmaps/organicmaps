#pragma once

#include "base/buffer_vector.hpp"

#include <optional>
#include <string_view>
#include <utility>

namespace indexer
{

class CustomKeyValue
{
  buffer_vector<std::pair<uint8_t, uint64_t>, 2> m_vals;

public:
  CustomKeyValue() = default;
  explicit CustomKeyValue(std::string_view buffer);

  void Add(uint8_t k, uint64_t v) { m_vals.emplace_back(k, v); }

  std::optional<uint64_t> Get(uint8_t k) const
  {
    for (auto const & v : m_vals)
      if (v.first == k)
        return v.second;
    return {};
  }

  uint64_t GetSure(uint8_t k) const
  {
    auto const res = Get(k);
    ASSERT(res, ());
    return *res;
  }

  std::string ToString() const;

  friend std::string DebugPrint(CustomKeyValue const & kv);
};

}  // namespace indexer
