#pragma once

#include "search/search_quality/sample.hpp"

#include "base/scope_guard.hpp"

#include <set>
#include <type_traits>
#include <vector>

class Edits
{
public:
  using Relevance = search::Sample::Result::Relevance;

  class RelevanceEditor
  {
  public:
    RelevanceEditor() = default;
    RelevanceEditor(Edits & parent, size_t index);

    bool IsValid() const { return m_parent != nullptr; }

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    Relevance Get() const;

  private:
    Edits * m_parent = nullptr;
    size_t m_index = 0;
  };

  struct Delegate
  {
    virtual ~Delegate() = default;
    virtual void OnUpdate() = 0;
  };

  Edits(Delegate & delegate);

  void ResetRelevances(std::vector<Relevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool UpdateRelevance(size_t index, Relevance relevance);

  std::vector<Relevance> const & GetRelevances() { return m_currRelevances; }

  void Clear();
  bool HasChanges() const;

private:
  template <typename Fn>
  typename std::result_of<Fn()>::type WithDelegate(Fn && fn)
  {
    MY_SCOPE_GUARD(cleanup, [this]() { m_delegate.OnUpdate(); });
    return fn();
  }

  Delegate & m_delegate;

  std::vector<Relevance> m_origRelevances;
  std::vector<Relevance> m_currRelevances;

  std::set<size_t> m_relevanceEdits;
};
