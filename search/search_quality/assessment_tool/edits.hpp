#pragma once

#include "search/search_quality/sample.hpp"

#include "base/scope_guard.hpp"

#include <cstddef>
#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

class Edits
{
public:
  struct Update
  {
    static auto constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    enum class Type
    {
      SingleRelevance,
      AllRelevances
    };

    static Update AllRelevancesUpdate() { return Update{}; }

    static Update SingleRelevanceUpdate(size_t index)
    {
      Update result;
      result.m_index = index;
      result.m_type = Type::SingleRelevance;
      return result;
    }

    size_t m_index = kInvalidIndex;
    Type m_type = Type::AllRelevances;
  };

  using OnUpdate = std::function<void(Update const & update)>;
  using Relevance = search::Sample::Result::Relevance;

  class RelevanceEditor
  {
  public:
    RelevanceEditor(Edits & parent, size_t index);

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    Relevance Get() const;
    bool HasChanges() const;

  private:
    Edits & m_parent;
    size_t m_index = 0;
  };

  explicit Edits(OnUpdate onUpdate) : m_onUpdate(onUpdate) {}

  void ResetRelevances(std::vector<Relevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool SetRelevance(size_t index, Relevance relevance);

  std::vector<Relevance> const & GetRelevances() const { return m_currRelevances; }

  void Clear();
  bool HasChanges() const;
  bool HasChanges(size_t index) const;

private:
  template <typename Fn>
  typename std::result_of<Fn()>::type WithObserver(Update const & update, Fn && fn)
  {
    MY_SCOPE_GUARD(cleanup, ([this, &update]() {
                     if (m_onUpdate)
                       m_onUpdate(update);
                   }));
    return fn();
  }

  std::vector<Relevance> m_origRelevances;
  std::vector<Relevance> m_currRelevances;

  size_t m_numEdits = 0;

  OnUpdate m_onUpdate;
};
