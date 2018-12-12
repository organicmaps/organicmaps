#include "search/search_quality/assessment_tool/main_model.hpp"

#include "search/feature_loader.hpp"
#include "search/search_quality/assessment_tool/view.hpp"
#include "search/search_quality/matcher.hpp"

#include "map/framework.hpp"

#include "search/search_params.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>

using namespace std;

// MainModel::SampleContext ------------------------------------------------------------------------

// MainModel ---------------------------------------------------------------------------------------
MainModel::MainModel(Framework & framework)
  : m_framework(framework)
  , m_dataSource(m_framework.GetDataSource())
  , m_loader(m_dataSource)
  , m_contexts(
        [this](size_t sampleIndex, Edits::Update const & update) {
          OnUpdate(View::ResultType::Found, sampleIndex, update);
        },
        [this](size_t sampleIndex, Edits::Update const & update) {
          OnUpdate(View::ResultType::NonFound, sampleIndex, update);
        })
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
  if (!search::Sample::DeserializeFromJSONLines(contents, samples))
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

  string contents;
  search::Sample::SerializeToJSONLines(m_contexts.MakeSamples(m_loader), contents);

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
  CHECK_LESS(static_cast<size_t>(index), m_contexts.Size(), ());
  CHECK(m_view, ());

  m_selectedSample = index;

  auto & context = m_contexts[index];
  auto const & sample = context.m_sample;
  m_view->ShowSample(index, sample, sample.m_posAvailable, sample.m_pos, context.HasChanges());

  ResetSearch();
  auto const timestamp = m_queryTimestamp;
  m_numShownResults = 0;

  if (context.m_initialized)
  {
    OnResults(timestamp, index, context.m_foundResults, context.m_foundResultsEdits.GetRelevances(),
              context.m_goldenMatching, context.m_actualMatching);
    return;
  }

  auto & engine = m_framework.GetSearchAPI().GetEngine();
  {
    search::SearchParams params;
    sample.FillSearchParams(params);
    params.m_onResults = [this, index, sample, timestamp](search::Results const & results) {
      vector<boost::optional<Edits::Relevance>> relevances;
      vector<size_t> goldenMatching;
      vector<size_t> actualMatching;

      if (results.IsEndedNormal())
      {
        // Can't use m_loader here due to thread-safety issues.
        search::FeatureLoader loader(m_dataSource);
        search::Matcher matcher(loader);

        vector<search::Result> const actual(results.begin(), results.end());
        matcher.Match(sample.m_results, actual, goldenMatching, actualMatching);
        relevances.resize(actual.size());
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

      GetPlatform().RunTask(Platform::Thread::Gui, bind(&MainModel::OnResults, this, timestamp, index, results,
                                                        relevances, goldenMatching, actualMatching));
    };

    m_queryHandle = engine.Search(params);
    m_view->OnSearchStarted();
  }
}

void MainModel::OnResultSelected(int index)
{
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());
  auto const & context = m_contexts[m_selectedSample];
  auto const & foundResults = context.m_foundResults;

  CHECK_GREATER_OR_EQUAL(index, 0, ());
  CHECK_LESS(static_cast<size_t>(index), foundResults.GetCount(), ());
  m_view->MoveViewportToResult(foundResults[index]);
}

void MainModel::OnNonFoundResultSelected(int index)
{
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());
  auto const & context = m_contexts[m_selectedSample];
  auto const & results = context.m_nonFoundResults;

  CHECK_GREATER_OR_EQUAL(index, 0, ());
  CHECK_LESS(static_cast<size_t>(index), results.size(), ());
  m_view->MoveViewportToResult(results[index]);
}

void MainModel::OnShowViewportClicked()
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());

  auto const & context = m_contexts[m_selectedSample];
  m_view->MoveViewportToRect(context.m_sample.m_viewport);
}

void MainModel::OnShowPositionClicked()
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());

  static int constexpr kViewportAroundTopResultsSizeM = 100;
  static double constexpr kViewportAroundTopResultsScale = 1.2;
  static size_t constexpr kMaxTopResults = 3;

  auto const & context = m_contexts[m_selectedSample];

  vector<m2::PointD> points;
  if (context.m_sample.m_posAvailable)
    points.push_back(context.m_sample.m_pos);

  size_t resultsAdded = 0;
  for (auto const & result : context.m_foundResults)
  {
    if (!result.HasPoint())
      continue;

    if (resultsAdded == kMaxTopResults)
      break;

    points.push_back(result.GetFeatureCenter());
    ++resultsAdded;
  }

  CHECK(!points.empty(), ());
  auto boundingBox = m2::ApplyCalculator(points, m2::CalculateBoundingBox());
  boundingBox.Scale(kViewportAroundTopResultsScale);

  auto const minRect = MercatorBounds::RectByCenterXYAndSizeInMeters(
      boundingBox.Center(), kViewportAroundTopResultsSizeM);
  m_view->MoveViewportToRect(m2::Add(boundingBox, minRect));
}

void MainModel::OnMarkAllAsRelevantClicked()
{
  OnChangeAllRelevancesClicked(Edits::Relevance::Relevant);
}

void MainModel::OnMarkAllAsIrrelevantClicked()
{
  OnChangeAllRelevancesClicked(Edits::Relevance::Irrelevant);
}

bool MainModel::HasChanges() { return m_contexts.HasChanges(); }

bool MainModel::AlreadyInSamples(FeatureID const & id)
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());

  bool found = false;
  ForAnyMatchingEntry(m_contexts[m_selectedSample], id, [&](Edits & edits, size_t index) {
    auto const & entry = edits.GetEntry(index);
    if (!entry.m_deleted)
      found = true;
  });
  return found;
}

void MainModel::AddNonFoundResult(FeatureID const & id)
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());

  auto & context = m_contexts[m_selectedSample];

  bool resurrected = false;
  ForAnyMatchingEntry(context, id, [&](Edits & edits, size_t index) {
    auto const & entry = edits.GetEntry(index);
    CHECK(entry.m_deleted, ());
    edits.Resurrect(index);
    resurrected = true;
  });
  if (resurrected)
    return;

  FeatureType ft;
  CHECK(m_loader.Load(id, ft), ("Can't load feature:", id));
  auto const result = search::Sample::Result::Build(ft, search::Sample::Result::Relevance::Vital);
  context.AddNonFoundResult(result);
}

void MainModel::OnUpdate(View::ResultType type, size_t sampleIndex, Edits::Update const & update)
{
  using Type = Edits::Update::Type;

  CHECK_GREATER_OR_EQUAL(sampleIndex, 0, ());
  CHECK_LESS(static_cast<size_t>(sampleIndex), m_contexts.Size(), ());

  auto & context = m_contexts[sampleIndex];

  if (update.m_type == Type::Add)
  {
    CHECK_EQUAL(type, View::ResultType::NonFound, ());
    m_view->ShowNonFoundResults(context.m_nonFoundResults,
                                context.m_nonFoundResultsEdits.GetEntries());
    m_view->SetEdits(m_selectedSample, context.m_foundResultsEdits, context.m_nonFoundResultsEdits);
  }

  m_view->OnResultChanged(sampleIndex, type, update);
  m_view->OnSampleChanged(sampleIndex, context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());

  if (update.m_type == Type::Add || update.m_type == Type::Resurrect ||
      update.m_type == Type::Delete)
  {
    CHECK(context.m_initialized, ());
    CHECK_EQUAL(type, View::ResultType::NonFound, ());
    ShowMarks(context);
  }
}

void MainModel::OnResults(uint64_t timestamp, size_t sampleIndex, search::Results const & results,
                          vector<boost::optional<Edits::Relevance>> const & relevances,
                          vector<size_t> const & goldenMatching,
                          vector<size_t> const & actualMatching)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());

  if (timestamp != m_queryTimestamp)
    return;

  CHECK_LESS_OR_EQUAL(m_numShownResults, results.GetCount(), ());
  m_view->AddFoundResults(results.begin() + m_numShownResults, results.end());
  m_numShownResults = results.GetCount();

  auto & context = m_contexts[sampleIndex];
  context.m_foundResults = results;

  if (!results.IsEndedNormal())
    return;

  if (!context.m_initialized)
  {
    context.m_foundResultsEdits.Reset(relevances);
    context.m_goldenMatching = goldenMatching;
    context.m_actualMatching = actualMatching;

    {
      vector<boost::optional<Edits::Relevance>> relevances;

      auto & nonFound = context.m_nonFoundResults;
      CHECK(nonFound.empty(), ());
      for (size_t i = 0; i < context.m_goldenMatching.size(); ++i)
      {
        auto const j = context.m_goldenMatching[i];
        if (j != search::Matcher::kInvalidId)
          continue;
        nonFound.push_back(context.m_sample.m_results[i]);
        relevances.emplace_back(nonFound.back().m_relevance);
      }
      context.m_nonFoundResultsEdits.Reset(relevances);
    }

    context.m_initialized = true;
  }

  m_view->ShowNonFoundResults(context.m_nonFoundResults,
                              context.m_nonFoundResultsEdits.GetEntries());
  ShowMarks(context);
  m_view->OnResultChanged(sampleIndex, View::ResultType::Found,
                          Edits::Update::MakeAll());
  m_view->OnResultChanged(sampleIndex, View::ResultType::NonFound,
                          Edits::Update::MakeAll());
  m_view->OnSampleChanged(sampleIndex, context.HasChanges());
  m_view->SetEdits(sampleIndex, context.m_foundResultsEdits, context.m_nonFoundResultsEdits);
  m_view->OnSearchCompleted();
}

void MainModel::ResetSearch()
{
  ++m_queryTimestamp;
  if (auto handle = m_queryHandle.lock())
    handle->Cancel();
}

void MainModel::ShowMarks(Context const & context)
{
  m_view->ClearSearchResultMarks();
  m_view->ShowFoundResultsMarks(context.m_foundResults.begin(), context.m_foundResults.end());
  m_view->ShowNonFoundResultsMarks(context.m_nonFoundResults,
                                   context.m_nonFoundResultsEdits.GetEntries());
}

void MainModel::OnChangeAllRelevancesClicked(Edits::Relevance relevance)
{
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());
  auto & context = m_contexts[m_selectedSample];

  context.m_foundResultsEdits.SetAllRelevances(relevance);
  context.m_nonFoundResultsEdits.SetAllRelevances(relevance);

  m_view->OnResultChanged(m_selectedSample, View::ResultType::Found, Edits::Update::MakeAll());
  m_view->OnResultChanged(m_selectedSample, View::ResultType::NonFound, Edits::Update::MakeAll());
  m_view->OnSampleChanged(m_selectedSample, context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());
}

template <typename Fn>
void MainModel::ForAnyMatchingEntry(Context & context, FeatureID const & id, Fn && fn)
{
  CHECK(context.m_initialized, ());

  auto const & foundResults = context.m_foundResults;
  CHECK_EQUAL(foundResults.GetCount(), context.m_foundResultsEdits.NumEntries(), ());
  for (size_t i = 0; i < foundResults.GetCount(); ++i)
  {
    auto const & result = foundResults[i];
    if (result.GetResultType() != search::Result::Type::Feature)
      continue;
    if (result.GetFeatureID() == id)
      return fn(context.m_foundResultsEdits, i);
  }

  FeatureType ft;
  CHECK(m_loader.Load(id, ft), ("Can't load feature:", id));
  search::Matcher matcher(m_loader);

  auto const & nonFoundResults = context.m_nonFoundResults;
  CHECK_EQUAL(nonFoundResults.size(), context.m_nonFoundResultsEdits.NumEntries(), ());
  for (size_t i = 0; i < nonFoundResults.size(); ++i)
  {
    auto const & result = context.m_nonFoundResults[i];
    if (matcher.Matches(result, ft))
      return fn(context.m_nonFoundResultsEdits, i);
  }
}
