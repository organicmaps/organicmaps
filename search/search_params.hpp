#pragma once

#include "search/bookmarks/types.hpp"
#include "search/filtering_params.hpp"
#include "search/mode.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace search
{
class Results;
class Tracer;

struct SearchParams
{
  static size_t constexpr kDefaultNumBookmarksResults = 1000;
  static size_t constexpr kDefaultBatchSizeEverywhere = 10;
  static size_t constexpr kDefaultNumResultsEverywhere = 30;
  static size_t constexpr kDefaultNumResultsInViewport = 200;
  static size_t constexpr kPreResultsCount = 200;

  using TimeDurationT = base::Timer::DurationT;
  /// @todo Short timeouts lead to a non-working search on slow devices. Design a better solution.
  static TimeDurationT constexpr kDefaultTimeout = std::chrono::seconds(8);

  using OnStarted = std::function<void()>;
  using OnResults = std::function<void(Results const &)>;

  bool IsEqualCommon(SearchParams const & rhs) const;

  void Clear() { m_query.clear(); }

  OnStarted m_onStarted;

  // This function may be called an arbitrary number of times during
  // the search session. The argument to every call except the first must contain
  // as its prefix the argument of the previous call, i.e. |m_onResults| is
  // always called for the whole array of results found so far but new results
  // are only appended to the end.
  //
  // The function may be called several times with the same arguments.
  // The reasoning is as follows: we could either 1) rearrange results
  // between calls if a better ranked result is found after the first
  // emit has happened or 2) only append as we do now but never call
  // the function twice with the same array of results.
  // We decided against the option 1) so as not to annoy the user by
  // rearranging.
  // We do not guarantee the option 2) because an efficient client
  // would only need to redraw the appended part and therefore would
  // be fast enough if the two calls are the same. The implementation of
  // the search may decide against duplicating calls but no guarantees are given.
  OnResults m_onResults;

  std::string m_query;
  std::string m_inputLocale;

  std::optional<m2::PointD> m_position;
  m2::RectD m_viewport;

  size_t m_batchSize = kDefaultBatchSizeEverywhere;
  size_t m_maxNumResults = kDefaultNumResultsEverywhere;

  // Minimal distance between search results (by x,y axes in mercator), needed for filtering of viewport search results.
  m2::PointD m_minDistanceOnMapBetweenResults{0, 0};

  RecommendedFilteringParams m_filteringParams;

  bookmarks::GroupId m_bookmarksGroupId = bookmarks::kInvalidGroupId;

  // Amount of time after which the search is aborted.
  TimeDurationT m_timeout = kDefaultTimeout;

  std::shared_ptr<Tracer> m_tracer;

  Mode m_mode = Mode::Everywhere;

  // Needed to generate search suggests.
  bool m_suggestsEnabled = false;

  // Needed to generate address for results.
  bool m_needAddress = false;

  // Needed to highlight matching parts of search result names.
  bool m_needHighlighting = false;

  // True if you need *pure* category results only, without names/addresses/etc matching.
  bool m_categorialRequest = false;

  // Set to true for debug logs and tests.
#ifdef DEBUG
  bool m_useDebugInfo = true;
#else   // RELEASE
  bool m_useDebugInfo = false;
#endif  // DEBUG
};

std::string DebugPrint(SearchParams const & params);
}  // namespace search
