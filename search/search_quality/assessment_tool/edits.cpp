#include "search/search_quality/assessment_tool/edits.hpp"

#include "base/assert.hpp"

// Edits::RelevanceEditor --------------------------------------------------------------------------
Edits::RelevanceEditor::RelevanceEditor(Edits & parent, size_t index)
  : m_parent(&parent), m_index(index)
{
}

bool Edits::RelevanceEditor::Set(Relevance relevance)
{
  CHECK(IsValid(), ());
  return m_parent->UpdateRelevance(m_index, relevance);
}

Edits::Relevance Edits::RelevanceEditor::Get() const
{
  CHECK(IsValid(), ());
  auto const & relevances = m_parent->GetRelevances();
  CHECK_LESS(m_index, relevances.size(), ());
  return relevances[m_index];
}

// Edits -------------------------------------------------------------------------------------------
Edits::Edits(Delegate & delegate) : m_delegate(delegate) {}

void Edits::ResetRelevances(std::vector<Relevance> const & relevances)
{
  WithDelegate([this, &relevances]() {
    m_origRelevances = relevances;
    m_currRelevances = relevances;
    m_relevanceEdits.clear();
  });
}

bool Edits::UpdateRelevance(size_t index, Relevance relevance)
{
  return WithDelegate([this, index, relevance]() {
    CHECK_LESS(index, m_currRelevances.size(), ());
    m_currRelevances[index] = relevance;

    CHECK_EQUAL(m_currRelevances.size(), m_origRelevances.size(), ());
    if (m_currRelevances[index] != m_origRelevances[index])
    {
      m_relevanceEdits.insert(index);
      return true;
    }
    else
    {
      m_relevanceEdits.erase(index);
      return false;
    }
  });
}

void Edits::Clear()
{
  WithDelegate([this]() {
    m_origRelevances.clear();
    m_currRelevances.clear();
    m_relevanceEdits.clear();
  });
}

bool Edits::HasChanges() const { return !m_relevanceEdits.empty(); }
