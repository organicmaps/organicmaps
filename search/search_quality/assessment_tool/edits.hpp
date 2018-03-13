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

  struct MaybeRelevance
  {
    MaybeRelevance() = default;
    MaybeRelevance(search::Sample::Result::Relevance relevance)
      : m_relevance(relevance), m_unknown(false)
    {
    }

    bool operator==(MaybeRelevance const & rhs) const
    {
      if (m_unknown && rhs.m_unknown)
        return true;
      if (m_unknown != rhs.m_unknown)
        return false;
      ASSERT(!m_unknown, ());
      ASSERT(!rhs.m_unknown, ());
      return m_relevance == rhs.m_relevance;
    }

    bool operator!=(MaybeRelevance const & rhs) const { return !(*this == rhs); }

    Relevance m_relevance = Relevance::Irrelevant;
    // The guard for |m_relevance|. The |m_relevance| field
    // should be read only when |m_unknown| is false.
    // It is implemented as a separate guard instead of another value
    // in the Relevance's enum because it results in cleaner typing
    // in our case (we only use unknown in the UI, so serializing
    // it makes no sense).
    bool m_unknown = true;
  };

  struct Entry
  {
    enum class Type
    {
      Loaded,
      Created
    };

    Entry() = default;
    Entry(MaybeRelevance relevance, Type type) : m_curr(relevance), m_orig(relevance), m_type(type)
    {
    }

    MaybeRelevance m_curr = {};
    MaybeRelevance m_orig = {};
    bool m_deleted = false;
    Type m_type = Type::Loaded;
  };

  struct Update
  {
    static auto constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    enum class Type
    {
      Single,
      All,
      Add,
      Delete,
      Resurrect
    };

    Update() = default;
    Update(Type type, size_t index): m_type(type), m_index(index) {}

    static Update MakeAll() { return {}; }
    static Update MakeSingle(size_t index) { return {Type::Single, index}; }
    static Update MakeAdd(size_t index) { return {Type::Add, index}; }
    static Update MakeDelete(size_t index) { return {Type::Delete, index}; }
    static Update MakeResurrect(size_t index) { return {Type::Resurrect, index}; }

    Type m_type = Type::All;
    size_t m_index = kInvalidIndex;
  };

  using OnUpdate = std::function<void(Update const & update)>;

  class Editor
  {
  public:
    Editor(Edits & parent, size_t index);

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    MaybeRelevance Get() const;
    bool HasChanges() const;
    Entry::Type GetType() const;

  private:
    Edits & m_parent;
    size_t m_index = 0;
  };

  explicit Edits(OnUpdate onUpdate) : m_onUpdate(onUpdate) {}

  void Apply();
  void Reset(std::vector<MaybeRelevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool SetRelevance(size_t index, Relevance relevance);

  // Sets relevances of all entries to |relevance|.
  void SetAllRelevances(Relevance relevance);

  // Adds a new entry.
  void Add(Relevance relevance);

  // Marks entry at |index| as deleted.
  void Delete(size_t index);

  // Resurrects previously deleted entry at |index|.
  void Resurrect(size_t index);

  std::vector<Entry> const & GetEntries() const { return m_entries; }
  Entry & GetEntry(size_t index);
  Entry const & GetEntry(size_t index) const;
  size_t NumEntries() const { return m_entries.size(); }
  std::vector<MaybeRelevance> GetRelevances() const;

  Entry const & Get(size_t index) const;


  void Clear();
  bool HasChanges() const;
  bool HasChanges(size_t index) const;

private:
  template <typename Fn>
  std::result_of_t<Fn()> WithObserver(Update const & update, Fn && fn)
  {
    MY_SCOPE_GUARD(obsCall, ([this, &update]() {
                     if (m_onUpdate)
                       m_onUpdate(update);
                   }));
    return fn();
  }

  std::vector<Entry> m_entries;

  size_t m_numEdits = 0;

  OnUpdate m_onUpdate;
};
