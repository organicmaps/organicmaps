#pragma once

#include "search/result.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace search
{
struct EverywhereSearchParams
{
  using OnResults = std::function<void(Results results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::optional<std::chrono::steady_clock::duration> m_timeout;
  bool m_isCategory = false;

  OnResults m_onResults;

  // Callers opt in; ordinary queries retain their existing ranking and house-number matching.
  bool m_prioritizeAddressMatches = false;
};
}  // namespace search
