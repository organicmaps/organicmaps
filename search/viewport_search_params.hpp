#pragma once

#include "std/function.hpp"
#include "std/string.hpp"

namespace search
{
struct ViewportSearchParams
{
  using TOnStarted = function<void()>;
  using TOnCompleted = function<void()>;

  string m_query;
  string m_inputLocale;

  TOnStarted m_onStarted;
  TOnCompleted m_onCompleted;
};
}  // namespace search
