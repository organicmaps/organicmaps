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
  using Relevance = search::Sample::Result::Relevance;

  struct Entry
  {
    Entry() = default;

    Relevance m_curr = Relevance::Irrelevant;
    Relevance m_orig = Relevance::Irrelevant;
    bool m_deleted = false;
  };

  struct Update
  {
    static auto constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    enum class Type
    {
      Single,
      All,
      Delete
    };

    Update() = default;
    Update(Type type, size_t index): m_type(type), m_index(index) {}

    static Update MakeAll() { return Update{}; }

    static Update MakeSingle(size_t index) { return Update{Type::Single, index}; }

    static Update MakeDelete(size_t index) { return Update{Type::Delete, index}; }

    Type m_type = Type::All;
    size_t m_index = kInvalidIndex;
  };

  using OnUpdate = std::function<void(Update const & update)>;

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

  void Apply();
  void Reset(std::vector<Relevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool SetRelevance(size_t index, Relevance relevance);

  // Marks entry at |index| as deleted.
  void Delete(size_t index);

  std::vector<Entry> const & GetEntries() const { return m_entries; }
  std::vector<Relevance> GetRelevances() const;

  Entry const & Get(size_t index) const;


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

  std::vector<Entry> m_entries;

  size_t m_numEdits = 0;

  OnUpdate m_onUpdate;
};
