#include "search/search_quality/assessment_tool/context.hpp"

#include "base/string_utils.hpp"

// Context -----------------------------------------------------------------------------------------
void Context::Clear()
{
  m_results.Clear();
  m_edits.Clear();
  m_initialized = false;
}

// ContextList -------------------------------------------------------------------------------------
ContextList::ContextList(OnUpdate onUpdate): m_onUpdate(onUpdate) {}

void ContextList::Resize(size_t size)
{
  size_t const oldSize = m_contexts.size();

  for (size_t i = size; i < oldSize; ++i)
    m_contexts[i].Clear();

  m_hasChanges.resize(size);
  for (size_t i = oldSize; i < size; ++i)
  {
    m_contexts.emplace_back([this, i]() {
      if (!m_hasChanges[i] && m_contexts[i].HasChanges())
        ++m_numChanges;
      if (m_hasChanges[i] && !m_contexts[i].HasChanges())
        --m_numChanges;
      m_hasChanges[i] = m_contexts[i].HasChanges();
      if (m_onUpdate)
        m_onUpdate(i);
    });
  }
}
