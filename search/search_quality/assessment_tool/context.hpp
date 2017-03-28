#pragma once

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include <cstddef>
#include <functional>
#include <vector>

struct Context
{
  explicit Context(Edits::OnUpdate onUpdate): m_edits(onUpdate) {}

  bool HasChanges() const { return m_initialized && m_edits.HasChanges(); }
  void Clear();

  search::Sample m_sample;
  search::Results m_results;
  Edits m_edits;
  bool m_initialized = false;
};

class ContextList
{
public:
  using OnUpdate = std::function<void(size_t index)>;

  explicit ContextList(OnUpdate onUpdate);

  void Resize(size_t size);
  size_t Size() const { return m_contexts.size(); }

  Context & operator[](size_t i) { return m_contexts[i]; }
  Context const & operator[](size_t i) const { return m_contexts[i]; }

  bool HasChanges() const { return m_numChanges != 0; }

private:
  std::vector<Context> m_contexts;
  std::vector<bool> m_hasChanges;
  size_t m_numChanges = 0;

  OnUpdate m_onUpdate;
};
