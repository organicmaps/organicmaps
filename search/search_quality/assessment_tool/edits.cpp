#include "search/search_quality/assessment_tool/edits.hpp"

#include "base/assert.hpp"

namespace
{
void UpdateNumEdits(Edits::Entry const & entry, Edits::MaybeRelevance const & r, size_t & numEdits)
{
  if (entry.m_curr != entry.m_orig && r == entry.m_orig)
    --numEdits;
  if (entry.m_curr == entry.m_orig && r != entry.m_orig)
    ++numEdits;
}
}  // namespace

// Edits::Editor -----------------------------------------------------------------------------------
Edits::Editor::Editor(Edits & parent, size_t index)
  : m_parent(parent), m_index(index)
{
}

bool Edits::Editor::Set(Relevance relevance)
{
  return m_parent.SetRelevance(m_index, relevance);
}

Edits::MaybeRelevance Edits::Editor::Get() const
{
  return m_parent.Get(m_index).m_curr;
}

bool Edits::Editor::HasChanges() const { return m_parent.HasChanges(m_index); }

Edits::Entry::Type Edits::Editor::GetType() const
{
  return m_parent.Get(m_index).m_type;
}

// Edits -------------------------------------------------------------------------------------------
void Edits::Apply()
{
  WithObserver(Update::MakeAll(), [this]() {
    for (auto & entry : m_entries)
    {
      entry.m_orig = entry.m_curr;
      entry.m_type = Entry::Type::Loaded;
    }
    m_numEdits = 0;
  });
}

void Edits::Reset(std::vector<MaybeRelevance> const & relevances)
{
  WithObserver(Update::MakeAll(), [this, &relevances]() {
    m_entries.resize(relevances.size());
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
      auto & entry = m_entries[i];
      entry.m_orig = relevances[i];
      entry.m_curr = relevances[i];
      entry.m_deleted = false;
      entry.m_type = Entry::Type::Loaded;
    }
    m_numEdits = 0;
  });
}

bool Edits::SetRelevance(size_t index, Relevance relevance)
{
  return WithObserver(Update::MakeSingle(index), [this, index, relevance]() {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];

    MaybeRelevance const r(relevance);

    UpdateNumEdits(entry, r, m_numEdits);

    entry.m_curr = r;
    return entry.m_curr != entry.m_orig;
  });
}

void Edits::SetAllRelevances(Relevance relevance)
{
  WithObserver(Update::MakeAll(), [this, relevance]() {
    for (auto & entry : m_entries)
    {
      MaybeRelevance const r(relevance);

      UpdateNumEdits(entry, r, m_numEdits);

      entry.m_curr = r;
    }
  });
}

void Edits::Add(Relevance relevance)
{
  auto const index = m_entries.size();
  WithObserver(Update::MakeAdd(index), [&]() {
    m_entries.emplace_back(relevance, Entry::Type::Created);
    ++m_numEdits;
  });
}

void Edits::Delete(size_t index)
{
  return WithObserver(Update::MakeDelete(index), [this, index]() {
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

void Edits::Resurrect(size_t index)
{
  return WithObserver(Update::MakeResurrect(index), [this, index]() {
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

Edits::Entry & Edits::GetEntry(size_t index)
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

Edits::Entry const & Edits::GetEntry(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

std::vector<Edits::MaybeRelevance> Edits::GetRelevances() const
{
  std::vector<MaybeRelevance> relevances(m_entries.size());
  for (size_t i = 0; i < m_entries.size(); ++i)
    relevances[i] = m_entries[i].m_curr;
  return relevances;
}

Edits::Entry const & Edits::Get(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  return m_entries[index];
}

void Edits::Clear()
{
  WithObserver(Update::MakeAll(), [this]() {
    m_entries.clear();
    m_numEdits = 0;
  });
}

bool Edits::HasChanges() const { return m_numEdits != 0; }

bool Edits::HasChanges(size_t index) const
{
  CHECK_LESS(index, m_entries.size(), ());
  auto const & entry = m_entries[index];
  return entry.m_curr != entry.m_orig;
}
