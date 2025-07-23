#pragma once

#include "search/search_quality/sample.hpp"

#include "base/scope_guard.hpp"

#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

struct SampleEdits
{
  using OnUpdate = std::function<void()>;

  explicit SampleEdits(OnUpdate onUpdate) : m_onUpdate(onUpdate) {}

  void Reset(bool origUseless);
  void FlipUsefulness();
  void Apply();
  bool HasChanges() const { return m_origUseless != m_currUseless; }
  void Clear() {}

  bool m_origUseless = false;
  bool m_currUseless = false;

  OnUpdate m_onUpdate;
};

class ResultsEdits
{
public:
  using Relevance = search::Sample::Result::Relevance;

  struct Entry
  {
    enum class Type
    {
      Loaded,
      Created
    };

    Entry() = default;
    Entry(std::optional<Relevance> relevance, Type type)
      : m_currRelevance(relevance)
      , m_origRelevance(relevance)
      , m_type(type)
    {}

    std::optional<Relevance> m_currRelevance = {};
    std::optional<Relevance> m_origRelevance = {};
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
    Update(Type type, size_t index) : m_type(type), m_index(index) {}

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
    Editor(ResultsEdits & parent, size_t index);

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    std::optional<Relevance> const & Get() const;
    bool HasChanges() const;
    Entry::Type GetType() const;

  private:
    ResultsEdits & m_parent;
    size_t m_index = 0;
  };

  explicit ResultsEdits(OnUpdate onUpdate) : m_onUpdate(onUpdate) {}

  void Apply();
  void Reset(std::vector<std::optional<Relevance>> const & relevances);

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
  std::vector<std::optional<Relevance>> GetRelevances() const;

  Entry const & Get(size_t index) const;

  void Clear();
  bool HasChanges() const;
  bool HasChanges(size_t index) const;

private:
  template <typename Fn>
  std::invoke_result_t<Fn> WithObserver(Update const & update, Fn && fn)
  {
    SCOPE_GUARD(obsCall, ([this, &update]()
    {
      if (m_onUpdate)
        m_onUpdate(update);
    }));
    return fn();
  }

  std::vector<Entry> m_entries;

  size_t m_numEdits = 0;

  OnUpdate m_onUpdate;
};
