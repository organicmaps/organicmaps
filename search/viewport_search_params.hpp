#pragma once

#include "search/hotels_filter.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"

namespace search
{
class Results;

struct ViewportSearchParams
{
  using TOnStarted = function<void()>;
  using TOnCompleted = function<void(Results const & results)>;

  string m_query;
  string m_inputLocale;
  shared_ptr<hotels_filter::Rule> m_hotelsFilter;

  TOnStarted m_onStarted;
  TOnCompleted m_onCompleted;
};
}  // namespace search
