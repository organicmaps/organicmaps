#include "search/search_quality/assessment_tool/context.hpp"

#include "search/feature_loader.hpp"
#include "search/search_quality/matcher.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace std;

// Context -----------------------------------------------------------------------------------------
void Context::Clear()
{
  m_goldenMatching.clear();
  m_actualMatching.clear();

  m_foundResults.Clear();
  m_foundResultsEdits.Clear();

  m_nonFoundResults.clear();
  m_nonFoundResultsEdits.Clear();

  m_sampleEdits.Clear();

  m_initialized = false;
}

void Context::LoadFromSample(search::Sample const & sample)
{
  Clear();
  m_sample = sample;
  m_sampleEdits.Reset(sample.m_useless);
}

search::Sample Context::MakeSample(search::FeatureLoader & loader) const
{
  search::Sample outSample = m_sample;

  if (!m_initialized)
    return outSample;

  auto const & foundEntries = m_foundResultsEdits.GetEntries();
  auto const & nonFoundEntries = m_nonFoundResultsEdits.GetEntries();

  auto & outResults = outSample.m_results;
  outResults.clear();

  CHECK_EQUAL(m_goldenMatching.size(), m_sample.m_results.size(), ());
  CHECK_EQUAL(m_actualMatching.size(), foundEntries.size(), ());
  CHECK_EQUAL(m_actualMatching.size(), m_foundResults.GetCount(), ());

  // Iterates over original (loaded from the file with search samples)
  // results first.
  size_t k = 0;
  for (size_t i = 0; i < m_sample.m_results.size(); ++i)
  {
    auto const j = m_goldenMatching[i];

    if (j == search::Matcher::kInvalidId)
    {
      auto const & entry = nonFoundEntries[k++];
      auto const deleted = entry.m_deleted;
      auto const & curr = entry.m_currRelevance;
      if (!deleted && curr)
      {
        auto result = m_sample.m_results[i];
        result.m_relevance = *curr;
        outResults.push_back(result);
      }
      continue;
    }

    if (!foundEntries[j].m_currRelevance)
      continue;

    auto result = m_sample.m_results[i];
    result.m_relevance = *foundEntries[j].m_currRelevance;
    outResults.push_back(std::move(result));
  }

  // Iterates over results retrieved during assessment.
  for (size_t i = 0; i < m_foundResults.GetCount(); ++i)
  {
    auto const j = m_actualMatching[i];
    if (j != search::Matcher::kInvalidId)
    {
      // This result was processed by the loop above.
      continue;
    }

    if (!foundEntries[i].m_currRelevance)
      continue;

    auto const & result = m_foundResults[i];
    // No need in non-feature results.
    if (result.GetResultType() != search::Result::Type::Feature)
      continue;

    auto ft = loader.Load(result.GetFeatureID());
    CHECK(ft, ());
    outResults.push_back(search::Sample::Result::Build(*ft, *foundEntries[i].m_currRelevance));
  }

  outSample.m_useless = m_sampleEdits.m_currUseless;

  return outSample;
}

void Context::ApplyEdits()
{
  if (!m_initialized)
    return;
  m_foundResultsEdits.Apply();
  m_nonFoundResultsEdits.Apply();
  m_sampleEdits.Apply();
}

// ContextList -------------------------------------------------------------------------------------
ContextList::ContextList(OnResultsUpdate onResultsUpdate, OnResultsUpdate onNonFoundResultsUpdate,
                         OnSampleUpdate onSampleUpdate)
  : m_onResultsUpdate(onResultsUpdate)
  , m_onNonFoundResultsUpdate(onNonFoundResultsUpdate)
  , m_onSampleUpdate(onSampleUpdate)
{}

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
    m_contexts.emplace_back(
        [this, i](ResultsEdits::Update const & update)
    {
      OnContextUpdated(i);
      if (m_onResultsUpdate)
        m_onResultsUpdate(i, update);
    },
        [this, i](ResultsEdits::Update const & update)
    {
      OnContextUpdated(i);
      if (m_onNonFoundResultsUpdate)
        m_onNonFoundResultsUpdate(i, update);
    }, [this, i]()
    {
      OnContextUpdated(i);
      if (m_onSampleUpdate)
        m_onSampleUpdate(i);
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

void ContextList::OnContextUpdated(size_t index)
{
  if (!m_hasChanges[index] && m_contexts[index].HasChanges())
    ++m_numChanges;
  if (m_hasChanges[index] && !m_contexts[index].HasChanges())
    --m_numChanges;
  m_hasChanges[index] = m_contexts[index].HasChanges();
}
