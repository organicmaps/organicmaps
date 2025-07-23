#include "search/search_quality/assessment_tool/edits.hpp"

#include "base/assert.hpp"

using namespace std;

namespace
{
void UpdateNumEdits(ResultsEdits::Entry const & entry, ResultsEdits::Relevance const & r, size_t & numEdits)
{
  if (entry.m_currRelevance != entry.m_origRelevance && r == entry.m_origRelevance)
  {
    CHECK_GREATER(numEdits, 0, ());
    --numEdits;
  }
  if (entry.m_currRelevance == entry.m_origRelevance && r != entry.m_origRelevance)
    ++numEdits;
}
}  // namespace

// SampleEdits -------------------------------------------------------------------------------------
void SampleEdits::Reset(bool origUseless)
{
  m_origUseless = origUseless;
  m_currUseless = origUseless;
}

void SampleEdits::FlipUsefulness()
{
  m_currUseless ^= true;
  if (m_onUpdate)
    m_onUpdate();
}

void SampleEdits::Apply()
{
  m_origUseless = m_currUseless;
  if (m_onUpdate)
    m_onUpdate();
}

// ResultsEdits::Editor ----------------------------------------------------------------------------
ResultsEdits::Editor::Editor(ResultsEdits & parent, size_t index) : m_parent(parent), m_index(index) {}

bool ResultsEdits::Editor::Set(Relevance relevance)
{
  return m_parent.SetRelevance(m_index, relevance);
}

optional<ResultsEdits::Relevance> const & ResultsEdits::Editor::Get() const
{
  return m_parent.Get(m_index).m_currRelevance;
}

bool ResultsEdits::Editor::HasChanges() const
{
  return m_parent.HasChanges(m_index);
}

ResultsEdits::Entry::Type ResultsEdits::Editor::GetType() const
{
  return m_parent.Get(m_index).m_type;
}

// ResultsEdits ------------------------------------------------------------------------------------
void ResultsEdits::Apply()
{
  WithObserver(Update::MakeAll(), [this]()
  {
    for (auto & entry : m_entries)
    {
      entry.m_origRelevance = entry.m_currRelevance;
      entry.m_type = Entry::Type::Loaded;
    }
    m_numEdits = 0;
  });
}

void ResultsEdits::Reset(vector<optional<ResultsEdits::Relevance>> const & relevances)
{
  WithObserver(Update::MakeAll(), [this, &relevances]()
  {
    m_entries.resize(relevances.size());
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
      auto & entry = m_entries[i];
      entry.m_origRelevance = relevances[i];
      entry.m_currRelevance = relevances[i];
      entry.m_deleted = false;
      entry.m_type = Entry::Type::Loaded;
    }
    m_numEdits = 0;
  });
}

bool ResultsEdits::SetRelevance(size_t index, Relevance relevance)
{
  return WithObserver(Update::MakeSingle(index), [this, index, relevance]()
  {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];

    UpdateNumEdits(entry, relevance, m_numEdits);

    entry.m_currRelevance = relevance;
    return entry.m_currRelevance != entry.m_origRelevance;
  });
}

void ResultsEdits::SetAllRelevances(Relevance relevance)
{
  WithObserver(Update::MakeAll(), [this, relevance]()
  {
    for (auto & entry : m_entries)
    {
      UpdateNumEdits(entry, relevance, m_numEdits);
      entry.m_currRelevance = relevance;
    }
  });
}

void ResultsEdits::Add(Relevance relevance)
{
  auto const index = m_entries.size();
  WithObserver(Update::MakeAdd(index), [&]()
  {
    m_entries.emplace_back(relevance, Entry::Type::Created);
    ++m_numEdits;
  });
}

void ResultsEdits::Delete(size_t index)
{
  return WithObserver(Update::MakeDelete(index), [this, index]()
  {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];
    CHECK(!entry.m_deleted, ());
    entry.m_deleted = true;
    switch (entry.m_type)
    {
    case Entry::Type::Loaded: ++m_numEdits; break;
    case Entry::Type::Created: --m_numEdits; break;
    }
  });
}

void ResultsEdits::Resurrect(size_t index)
{
  return WithObserver(Update::MakeResurrect(index), [this, index]()
  {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];
    CHECK(entry.m_deleted, ());
    CHECK_GREATER(m_numEdits, 0, ());
    entry.m_deleted = false;
    switch (entry.m_type)
    {
    case Entry::Type::Loaded: --m_numEdits; break;
    case Entry::Type::Created: ++m_numEdits; break;
    }
  });
}

ResultsEdits::Entry & ResultsEdits::GetEntry(size_t index)
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

ResultsEdits::Entry const & ResultsEdits::GetEntry(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

vector<optional<ResultsEdits::Relevance>> ResultsEdits::GetRelevances() const
{
  vector<optional<ResultsEdits::Relevance>> relevances(m_entries.size());
  for (size_t i = 0; i < m_entries.size(); ++i)
    relevances[i] = m_entries[i].m_currRelevance;
  return relevances;
}

ResultsEdits::Entry const & ResultsEdits::Get(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

void ResultsEdits::Clear()
{
  WithObserver(Update::MakeAll(), [this]()
  {
    m_entries.clear();
    m_numEdits = 0;
  });
}

bool ResultsEdits::HasChanges() const
{
  return m_numEdits != 0;
}

bool ResultsEdits::HasChanges(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  auto const & entry = m_entries[index];
  return entry.m_currRelevance != entry.m_origRelevance;
}
