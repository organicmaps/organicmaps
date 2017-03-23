#pragma once

#include "search/search_quality/sample.hpp"

#include "base/scope_guard.hpp"

#include <cstddef>
#include <type_traits>
#include <vector>

class Edits
{
public:
  using Relevance = search::Sample::Result::Relevance;

  class RelevanceEditor
  {
  public:
    RelevanceEditor(Edits & parent, size_t index);

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    Relevance Get() const;

  private:
    Edits & m_parent;
    size_t m_index = 0;
  };

  struct Observer
  {
    virtual ~Observer() = default;
    virtual void OnUpdate() = 0;
  };

  explicit Edits(Observer & observer);

  void ResetRelevances(std::vector<Relevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool UpdateRelevance(size_t index, Relevance relevance);

  std::vector<Relevance> const & GetRelevances() { return m_currRelevances; }

  void Clear();
  bool HasChanges() const;

private:
  template <typename Fn>
  typename std::result_of<Fn()>::type WithObserver(Fn && fn)
  {
    MY_SCOPE_GUARD(cleanup, [this]() { m_observer.OnUpdate(); });
    return fn();
  }

  Observer & m_observer;

  std::vector<Relevance> m_origRelevances;
  std::vector<Relevance> m_currRelevances;

  size_t m_numEdits = 0;
};
