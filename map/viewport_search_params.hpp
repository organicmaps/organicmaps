#pragma once

#include "search/result.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace search
{
struct ViewportSearchParams
{
  using OnStarted = std::function<void()>;
  using OnCompleted = std::function<void(Results results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::optional<std::chrono::steady_clock::duration> m_timeout;
  bool m_isCategory = false;

  OnStarted m_onStarted;
  OnCompleted m_onCompleted;
};
}  // namespace search
