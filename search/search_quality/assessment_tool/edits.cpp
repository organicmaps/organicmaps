#include "search/search_quality/assessment_tool/edits.hpp"

#include "base/assert.hpp"

// Edits::RelevanceEditor --------------------------------------------------------------------------
Edits::RelevanceEditor::RelevanceEditor(Edits & parent, size_t index)
  : m_parent(parent), m_index(index)
{
}

bool Edits::RelevanceEditor::Set(Relevance relevance)
{
  return m_parent.SetRelevance(m_index, relevance);
}

Edits::Relevance Edits::RelevanceEditor::Get() const
{
  return m_parent.Get(m_index).m_curr;
}

bool Edits::RelevanceEditor::HasChanges() const { return m_parent.HasChanges(m_index); }

// Edits::ResultDeleter ----------------------------------------------------------------------------
Edits::ResultDeleter::ResultDeleter(Edits & parent, size_t index) : m_parent(parent), m_index(index)
{
}

void Edits::ResultDeleter::Delete() { m_parent.Delete(m_index); }

// Edits -------------------------------------------------------------------------------------------
void Edits::Apply()
{
  WithObserver(Update::MakeAll(), [this]() {
    for (auto & entry : m_entries)
      entry.m_curr = entry.m_orig;
    m_numEdits = 0;
  });
}

void Edits::Reset(std::vector<Relevance> const & relevances)
{
  WithObserver(Update::MakeAll(), [this, &relevances]() {
    m_entries.resize(relevances.size());
    for (size_t i = 0; i < relevances.size(); ++i)
    {
      m_entries[i].m_orig = relevances[i];
      m_entries[i].m_curr = relevances[i];
      m_entries[i].m_deleted = false;
    }
    m_numEdits = 0;
  });
}

bool Edits::SetRelevance(size_t index, Relevance relevance)
{
  return WithObserver(Update::MakeSingle(index), [this, index, relevance]() {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];

    if (entry.m_curr != entry.m_orig && relevance == entry.m_orig)
      --m_numEdits;
    else if (entry.m_curr == entry.m_orig && relevance != entry.m_orig)
      ++m_numEdits;

    entry.m_curr = relevance;
    return entry.m_curr != entry.m_orig;
  });
}

void Edits::Delete(size_t index)
{
  return WithObserver(Update::MakeDelete(index), [this, index]() {
    CHECK_LESS(index, m_entries.size(), ());

    auto & entry = m_entries[index];
    CHECK(!entry.m_deleted, ());
    entry.m_deleted = true;
    ++m_numEdits;
  });
}

std::vector<Edits::Relevance> Edits::GetRelevances() const
{
  std::vector<Relevance> relevances(m_entries.size());
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
