#pragma once

#include "geometry/rect2d.hpp"

#include "search/result.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

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
  TestSearchRequest(TestSearchEngine & engine, string const & query, string const & locale,
                    m2::RectD const & viewport);

  void Wait();

  vector<search::Result> const & Results() const;

private:
  void Done(search::Results const & results);

  condition_variable m_cv;
  mutable mutex m_mu;

  vector<search::Result> m_results;
  bool m_done;
};
}  // namespace tests_support
}  // namespace search
