#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/queue.hpp"

class FeatureType;

namespace search
{
  class Result
  {
  public:
    Result(string const & name, m2::RectD const & rect)
      : m_name(name), m_rect(rect) {}
    string m_name;
    m2::RectD m_rect;
  };

  typedef pair<int, search::Result> elem_type;
  struct QueueComparer
  {
    // custom comparison for priority_queue
    bool operator() (elem_type const & e1, elem_type const & e2) const
    {
      return e1.first < e2.first;
    }
  };

  class Query
  {
    vector<string> m_tokens;
    /// custom comparison, holds found features
    priority_queue<elem_type, vector<elem_type>, QueueComparer> m_queue;

    // @TODO refactor and remove
    FeatureType const * m_currFeature;

  public:
    Query(string const & line);

    bool operator()(char lang, string const & utf8s);

    void Match(FeatureType const & f);
    template <class T> void ForEachResultRef(T & f)
    {
      while (!m_queue.empty())
      {
        f(m_queue.top().second);
        m_queue.pop();
      }
    }
  };

  class Processor
  {
    /// mutable here because indexer stores const ref to functor
    mutable Query & m_query;

  public:
    Processor(Query & query);
    bool operator() (FeatureType const & f) const;
  };
}
