#pragma once

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/matcher.hpp"
#include "search/search_quality/sample.hpp"

#include "base/string_utils.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace search
{
class FeatureLoader;
}

struct Context
{
  Context(Edits::OnUpdate onFoundResultsUpdate, Edits::OnUpdate onNonFoundResultsUpdate)
    : m_foundResultsEdits(onFoundResultsUpdate), m_nonFoundResultsEdits(onNonFoundResultsUpdate)
  {
  }

  void AddNonFoundResult(search::Sample::Result const & result)
  {
    CHECK_EQUAL(m_goldenMatching.size(), m_sample.m_results.size(), ());

    m_sample.m_results.push_back(result);
    m_goldenMatching.push_back(search::Matcher::kInvalidId);

    m_nonFoundResults.push_back(result);
    m_nonFoundResultsEdits.Add(result.m_relevance);
  }

  bool HasChanges() const
  {
    if (!m_initialized)
      return false;
    return m_foundResultsEdits.HasChanges() || m_nonFoundResultsEdits.HasChanges();
  }

  void Clear();

  // Makes sample in accordance with uncommited edits.
  search::Sample MakeSample(search::FeatureLoader & loader) const;

  // Commits all edits.
  void ApplyEdits();

  search::Sample m_sample;
  search::Results m_foundResults;
  Edits m_foundResultsEdits;

  std::vector<size_t> m_goldenMatching;
  std::vector<size_t> m_actualMatching;

  std::vector<search::Sample::Result> m_nonFoundResults;
  Edits m_nonFoundResultsEdits;

  bool m_initialized = false;
};

class ContextList
{
public:
  class SamplesSlice
  {
  public:
    SamplesSlice() = default;
    explicit SamplesSlice(ContextList const & contexts) : m_contexts(&contexts) {}

    bool IsValid() const { return m_contexts != nullptr; }

    std::string GetLabel(size_t index) const
    {
      return strings::ToUtf8((*m_contexts)[index].m_sample.m_query);
    }

    bool IsChanged(size_t index) const { return (*m_contexts)[index].HasChanges(); }

    size_t Size() const { return m_contexts->Size(); }

  private:
    ContextList const * m_contexts = nullptr;
  };

  using OnUpdate = std::function<void(size_t index, Edits::Update const & update)>;

  ContextList(OnUpdate onResultsUpdate, OnUpdate onNonFoundResultsUpdate);

  void Resize(size_t size);
  size_t Size() const { return m_contexts.size(); }

  Context & operator[](size_t i) { return m_contexts[i]; }
  Context const & operator[](size_t i) const { return m_contexts[i]; }

  bool HasChanges() const { return m_numChanges != 0; }

  // Generates search samples in accordance with uncommited edits.
  std::vector<search::Sample> MakeSamples(search::FeatureLoader & loader) const;

  // Commits all edits.
  void ApplyEdits();

private:
  void OnContextUpdated(size_t index);

  std::vector<Context> m_contexts;
  std::vector<bool> m_hasChanges;
  size_t m_numChanges = 0;

  OnUpdate m_onResultsUpdate;
  OnUpdate m_onNonFoundResultsUpdate;
};
