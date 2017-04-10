#include "search/search_quality/assessment_tool/context.hpp"

#include "search/feature_loader.hpp"
#include "search/search_quality/matcher.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace std;

// Context -----------------------------------------------------------------------------------------
void Context::Clear()
{
  m_results.Clear();
  m_edits.Clear();
  m_initialized = false;
}

search::Sample Context::MakeSample(search::FeatureLoader & loader) const
{
  search::Sample outSample = m_sample;

  if (!m_initialized)
    return outSample;

  auto const & relevances = m_edits.GetRelevances();

  auto & outResults = outSample.m_results;
  outResults.clear();

  CHECK_EQUAL(m_goldenMatching.size(), m_sample.m_results.size(), ());
  CHECK_EQUAL(m_actualMatching.size(), relevances.size(), ());
  CHECK_EQUAL(m_actualMatching.size(), m_results.GetCount(), ());

  // Iterates over original (loaded from the file with search samples)
  // results first.
  for (size_t i = 0; i < m_sample.m_results.size(); ++i)
  {
    auto const j = m_goldenMatching[i];

    // Some results weren't matched, so they weren't displayed to the
    // assessor. But we want to keep them.
    if (j == search::Matcher::kInvalidId)
    {
      outResults.push_back(m_sample.m_results[i]);
      continue;
    }

    // No need to keep irrelevant results.
    if (relevances[j] == search::Sample::Result::Relevance::Irrelevant)
      continue;

    auto result = m_sample.m_results[i];
    result.m_relevance = relevances[j];
    outResults.push_back(move(result));
  }

  // Iterates over results retrieved during assessment.
  for (size_t i = 0; i < m_results.GetCount(); ++i)
  {
    auto const j = m_actualMatching[i];
    if (j != search::Matcher::kInvalidId)
    {
      // This result was processed by the loop above.
      continue;
    }

    // No need to keep irrelevant results.
    if (relevances[i] == search::Sample::Result::Relevance::Irrelevant)
      continue;

    auto const & result = m_results.GetResult(i);
    // No need in non-feature results.
    if (result.GetResultType() != search::Result::RESULT_FEATURE)
      continue;

    FeatureType ft;
    CHECK(loader.Load(result.GetFeatureID(), ft), ());
    outResults.push_back(search::Sample::Result::Build(ft, relevances[i]));
  }

  return outSample;
}

void Context::ApplyEdits()
{
  if (!m_initialized)
    return;
  m_edits.ResetRelevances(m_edits.GetRelevances());
}

// ContextList -------------------------------------------------------------------------------------
ContextList::ContextList(OnUpdate onUpdate): m_onUpdate(onUpdate) {}

void ContextList::Resize(size_t size)
{
  size_t const oldSize = m_contexts.size();

  for (size_t i = size; i < oldSize; ++i)
    m_contexts[i].Clear();
  if (size < m_contexts.size())
    m_contexts.erase(m_contexts.begin() + size, m_contexts.end());

  m_hasChanges.resize(size);
  for (size_t i = oldSize; i < size; ++i)
  {
    m_contexts.emplace_back([this, i](Edits::Update const & update) {
      if (!m_hasChanges[i] && m_contexts[i].HasChanges())
        ++m_numChanges;
      if (m_hasChanges[i] && !m_contexts[i].HasChanges())
        --m_numChanges;
      m_hasChanges[i] = m_contexts[i].HasChanges();
      if (m_onUpdate)
        m_onUpdate(i, update);
    });
  }
}

vector<search::Sample> ContextList::MakeSamples(search::FeatureLoader & loader) const
{
  vector<search::Sample> samples;
  for (auto const & context : m_contexts)
    samples.push_back(context.MakeSample(loader));
  return samples;
}

void ContextList::ApplyEdits()
{
  for (auto & context : m_contexts)
    context.ApplyEdits();
}
