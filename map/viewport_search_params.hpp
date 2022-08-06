#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace search
{
class Results;

struct ViewportSearchParams
{
  using OnStarted = std::function<void()>;
  using OnCompleted = std::function<void(Results const & results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::optional<std::chrono::steady_clock::duration> m_timeout;
  bool m_isCategory = false;

  OnStarted m_onStarted;
  OnCompleted m_onCompleted;
};
}  // namespace search
