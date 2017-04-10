#include "search/search_quality/assessment_tool/main_model.hpp"

#include "search/feature_loader.hpp"
#include "search/search_quality/assessment_tool/view.hpp"
#include "search/search_quality/matcher.hpp"

#include "map/framework.hpp"

#include "search/search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>

using Relevance = search::Sample::Result::Relevance;
using namespace std;

// MainModel::SampleContext ------------------------------------------------------------------------

// MainModel ---------------------------------------------------------------------------------------
MainModel::MainModel(Framework & framework)
  : m_framework(framework)
  , m_index(m_framework.GetIndex())
  , m_contexts([this](size_t index, Edits::Update const & update) { OnUpdate(index, update); })
{
}

void MainModel::Open(string const & path)
{
  CHECK(m_view, ());

  string contents;

  {
    ifstream ifs(path);
    if (!ifs)
    {
      m_view->ShowError("Can't open file: " + path);
      return;
    }
    contents.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
  }

  vector<search::Sample> samples;
  if (!search::Sample::DeserializeFromJSON(contents, samples))
  {
    m_view->ShowError("Can't parse samples: " + path);
    return;
  }

  ResetSearch();

  m_view->Clear();

  m_contexts.Resize(samples.size());
  for (size_t i = 0; i < samples.size(); ++i)
  {
    auto & context = m_contexts[i];
    context.Clear();
    context.m_sample = samples[i];
  }
  m_path = path;

  m_view->SetSamples(ContextList::SamplesSlice(m_contexts));
  m_selectedSample = kInvalidIndex;
}

void MainModel::Save()
{
  CHECK(HasChanges(), ());
  SaveAs(m_path);
}

void MainModel::SaveAs(string const & path)
{
  CHECK(HasChanges(), ());
  CHECK(!path.empty(), ());

  search::FeatureLoader loader(m_index);

  string contents;
  search::Sample::SerializeToJSON(m_contexts.MakeSamples(loader), contents);

  {
    ofstream ofs(path);
    if (!ofs)
    {
      m_view->ShowError("Can't open file: " + path);
      return;
    }
    copy(contents.begin(), contents.end(), ostreambuf_iterator<char>(ofs));
  }

  m_contexts.ApplyEdits();
  m_path = path;
}

void MainModel::OnSampleSelected(int index)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());

  CHECK_GREATER_OR_EQUAL(index, 0, ());
  CHECK_LESS(index, m_contexts.Size(), ());
  CHECK(m_view, ());

  m_selectedSample = index;

  auto & context = m_contexts[index];
  auto const & sample = context.m_sample;
  m_view->ShowSample(index, sample, context.HasChanges());

  ResetSearch();
  auto const timestamp = m_queryTimestamp;
  m_numShownResults = 0;

  if (context.m_initialized)
  {
    OnResults(timestamp, index, context.m_results, context.m_edits.GetRelevances(),
              context.m_goldenMatching, context.m_actualMatching);
    return;
  }

  auto & engine = m_framework.GetSearchEngine();
  {
    auto latLon = MercatorBounds::ToLatLon(sample.m_pos);

    search::SearchParams params;
    params.m_query = strings::ToUtf8(sample.m_query);
    params.m_inputLocale = sample.m_locale;
    params.m_suggestsEnabled = false;
    params.SetPosition(latLon.lat, latLon.lon);

    params.m_onResults = [this, index, sample, timestamp](search::Results const & results) {
      vector<Relevance> relevances;
      vector<size_t> goldenMatching;
      vector<size_t> actualMatching;

      if (results.IsEndedNormal())
      {
        search::FeatureLoader loader(m_index);
        search::Matcher matcher(loader);

        vector<search::Result> const actual(results.begin(), results.end());
        matcher.Match(sample.m_results, actual, goldenMatching, actualMatching);
        relevances.assign(actual.size(), Relevance::Irrelevant);
        for (size_t i = 0; i < goldenMatching.size(); ++i)
        {
          auto const j = goldenMatching[i];
          if (j != search::Matcher::kInvalidId)
          {
            CHECK_LESS(j, relevances.size(), ());
            relevances[j] = sample.m_results[i].m_relevance;
          }
        }
      }

      GetPlatform().RunOnGuiThread(bind(&MainModel::OnResults, this, timestamp, index, results,
                                        relevances, goldenMatching, actualMatching));
    };

    m_queryHandle = engine.Search(params, sample.m_viewport);
  }
}

void MainModel::OnResultSelected(int index)
{
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(m_selectedSample, m_contexts.Size(), ());
  auto const & context = m_contexts[m_selectedSample];
  auto const & results = context.m_results;

  CHECK_GREATER_OR_EQUAL(index, 0, ());
  CHECK_LESS(index, results.GetCount(), ());
  m_view->MoveViewportToResult(results.GetResult(index));
}

void MainModel::OnShowViewportClicked()
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK(m_selectedSample < m_contexts.Size(), ());

  auto const & context = m_contexts[m_selectedSample];
  m_view->MoveViewportToRect(context.m_sample.m_viewport);
}

void MainModel::OnShowPositionClicked()
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK(m_selectedSample < m_contexts.Size(), ());

  static int constexpr kViewportAroundPositionSizeM = 100;

  auto const & context = m_contexts[m_selectedSample];
  auto const & position = context.m_sample.m_pos;
  auto const rect =
      MercatorBounds::RectByCenterXYAndSizeInMeters(position, kViewportAroundPositionSizeM);
  m_view->MoveViewportToRect(rect);
}

bool MainModel::HasChanges() { return m_contexts.HasChanges(); }

void MainModel::OnUpdate(size_t index, Edits::Update const & update)
{
  CHECK_LESS(index, m_contexts.Size(), ());
  auto & context = m_contexts[index];
  m_view->OnSampleChanged(index, update, context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());
}

void MainModel::OnResults(uint64_t timestamp, size_t index, search::Results const & results,
                          vector<Relevance> const & relevances,
                          vector<size_t> const & goldenMatching,
                          vector<size_t> const & actualMatching)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());

  if (timestamp != m_queryTimestamp)
    return;

  CHECK_LESS_OR_EQUAL(m_numShownResults, results.GetCount(), ());
  m_view->ShowResults(results.begin() + m_numShownResults, results.end());
  m_numShownResults = results.GetCount();

  auto & context = m_contexts[index];
  context.m_results = results;

  if (!results.IsEndedNormal())
    return;

  if (!context.m_initialized)
  {
    context.m_edits.ResetRelevances(relevances);
    context.m_goldenMatching = goldenMatching;
    context.m_actualMatching = actualMatching;
    context.m_initialized = true;
  }
  m_view->OnSampleChanged(index, Edits::Update::AllRelevancesUpdate(), context.HasChanges());
  m_view->EnableSampleEditing(index, context.m_edits);
}

void MainModel::ResetSearch()
{
  ++m_queryTimestamp;
  if (auto handle = m_queryHandle.lock())
    handle->Cancel();
}
