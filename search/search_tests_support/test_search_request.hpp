#pragma once

#include "geometry/rect2d.hpp"

#include "search/result.hpp"
#include "search/search_params.hpp"

#include "base/timer.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace search
{
namespace tests_support
{
class TestSearchEngine;

// This class wraps a search query to a search engine. Note that the
// last token will be handled as a complete token iff it will be
// followed by a space in a query.
class TestSearchRequest
{
public:
  inline static double const kDefaultTestStreetSearchRadiusM = 2e7;
  inline static double const kDefaultTestVillageSearchRadiusM = 2e7;

  TestSearchRequest(TestSearchEngine & engine, std::string const & query,
                    std::string const & locale, Mode mode, m2::RectD const & viewport);
  TestSearchRequest(TestSearchEngine & engine, SearchParams const & params);

  // Initiates the search and waits for it to finish.
  void Run();

  // Initiates asynchronous search.
  void Start();

  // Waits for the search to finish.
  void Wait();

  // Call these functions only after call to Wait().
  std::chrono::steady_clock::duration ResponseTime() const;
  std::vector<search::Result> const & Results() const;

protected:
  TestSearchRequest(TestSearchEngine & engine, std::string const & query,
                    std::string const & locale, Mode mode, m2::RectD const & viewport,
                    SearchParams::OnStarted const & onStarted,
                    SearchParams::OnResults const & onResults);

  void SetUpCallbacks();
  void SetUpResultParams();

  void OnStarted();
  void OnResults(search::Results const & results);

  // Overrides the default onResults callback.
  void SetCustomOnResults(SearchParams::OnResults const & onResults);

  std::condition_variable m_cv;
  mutable std::mutex m_mu;

  std::vector<search::Result> m_results;
  bool m_done = false;

  base::Timer m_timer;
  std::chrono::steady_clock::duration m_startTime;
  std::chrono::steady_clock::duration m_endTime;

  TestSearchEngine & m_engine;
  SearchParams m_params;
};
}  // namespace tests_support
}  // namespace search
