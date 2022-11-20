#include "main_model.hpp"
#include "view.hpp"

#include "search/feature_loader.hpp"
#include "search/search_params.hpp"
#include "search/search_quality/helpers.hpp"
#include "search/search_quality/matcher.hpp"

#include "map/framework.hpp"

#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

using namespace std;

// MainModel ---------------------------------------------------------------------------------------
MainModel::MainModel(Framework & framework)
  : m_framework(framework)
  , m_dataSource(m_framework.GetDataSource())
  , m_loader(m_dataSource)
  , m_contexts(
        [this](size_t sampleIndex, ResultsEdits::Update const & update) {
          OnUpdate(View::ResultType::Found, sampleIndex, update);
        },
        [this](size_t sampleIndex, ResultsEdits::Update const & update) {
          OnUpdate(View::ResultType::NonFound, sampleIndex, update);
        },
        [this](size_t sampleIndex) { OnSampleUpdate(sampleIndex); })
  , m_runner(m_framework, m_dataSource, m_contexts,
             [this](search::Results const & results) { UpdateViewOnResults(results); },
             [this](size_t index) {
               // Only the first parameter matters because we only change SearchStatus.
               m_view->OnSampleChanged(index, false /* isUseless */, false /* hasEdits */);
             })
{
  search::search_quality::CheckLocale();
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

  m_runner.ResetForegroundSearch();
  m_runner.ResetBackgroundSearch();

  m_view->Clear();

  m_contexts.Resize(samples.size());
  for (size_t i = 0; i < samples.size(); ++i)
    m_contexts[i].LoadFromSample(samples[i]);

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

void MainModel::InitiateBackgroundSearch(size_t from, size_t to)
{
  m_runner.InitiateBackgroundSearch(from, to);
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

  m_view->ShowSample(index, sample, sample.m_pos, context.IsUseless(), context.HasChanges());

  m_runner.ResetForegroundSearch();

  if (context.m_initialized)
  {
    UpdateViewOnResults(context.m_foundResults);
    return;
  }

  InitiateForegroundSearch(index);
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
  if (context.m_sample.m_pos)
    points.push_back(*context.m_sample.m_pos);

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

  auto const minRect =
      mercator::RectByCenterXYAndSizeInMeters(boundingBox.Center(), kViewportAroundTopResultsSizeM);
  m_view->MoveViewportToRect(m2::Add(boundingBox, minRect));
}

void MainModel::OnMarkAllAsRelevantClicked()
{
  OnChangeAllRelevancesClicked(ResultsEdits::Relevance::Relevant);
}

void MainModel::OnMarkAllAsIrrelevantClicked()
{
  OnChangeAllRelevancesClicked(ResultsEdits::Relevance::Irrelevant);
}

bool MainModel::HasChanges() { return m_contexts.HasChanges(); }

bool MainModel::AlreadyInSamples(FeatureID const & id)
{
  CHECK(m_selectedSample != kInvalidIndex, ());
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());

  bool found = false;
  ForAnyMatchingEntry(m_contexts[m_selectedSample], id, [&](ResultsEdits & edits, size_t index) {
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
  ForAnyMatchingEntry(context, id, [&](ResultsEdits & edits, size_t index) {
    auto const & entry = edits.GetEntry(index);
    CHECK(entry.m_deleted, ());
    edits.Resurrect(index);
    resurrected = true;
  });
  if (resurrected)
    return;

  auto ft = m_loader.Load(id);
  CHECK(ft, ("Can't load feature:", id));
  auto const result = search::Sample::Result::Build(*ft, search::Sample::Result::Relevance::Vital);
  context.AddNonFoundResult(result);
}

void MainModel::FlipSampleUsefulness(int index)
{
  CHECK_EQUAL(m_selectedSample, index, ());

  m_contexts[index].m_sampleEdits.FlipUsefulness();

  // Don't bother with resetting search: we cannot tell whether
  // the sample is useless without its results anyway.
}

void MainModel::InitiateForegroundSearch(size_t index)
{
  auto & context = m_contexts[index];
  auto const & sample = context.m_sample;

  m_view->ShowSample(index, sample, sample.m_pos, context.IsUseless(), context.HasChanges());
  m_runner.InitiateForegroundSearch(index);
  m_view->OnSearchStarted();
}

void MainModel::OnUpdate(View::ResultType type, size_t sampleIndex,
                         ResultsEdits::Update const & update)
{
  using Type = ResultsEdits::Update::Type;

  CHECK_LESS(sampleIndex, m_contexts.Size(), ());
  auto & context = m_contexts[sampleIndex];

  if (update.m_type == Type::Add)
  {
    CHECK_EQUAL(type, View::ResultType::NonFound, ());
    m_view->ShowNonFoundResults(context.m_nonFoundResults,
                                context.m_nonFoundResultsEdits.GetEntries());
    m_view->SetResultsEdits(m_selectedSample, context.m_foundResultsEdits,
                            context.m_nonFoundResultsEdits);
  }

  m_view->OnResultChanged(sampleIndex, type, update);
  m_view->OnSampleChanged(sampleIndex, context.IsUseless(), context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());

  if (update.m_type == Type::Add || update.m_type == Type::Resurrect ||
      update.m_type == Type::Delete)
  {
    CHECK(context.m_initialized, ());
    CHECK_EQUAL(type, View::ResultType::NonFound, ());
    m_view->ShowMarks(context);
  }
}

void MainModel::OnSampleUpdate(size_t sampleIndex)
{
  auto & context = m_contexts[sampleIndex];

  m_view->OnSampleChanged(sampleIndex, context.IsUseless(), context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());
}

void MainModel::UpdateViewOnResults(search::Results const & results)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());

  m_view->AddFoundResults(results);

  if (!results.IsEndedNormal())
    return;

  auto & context = m_contexts[m_selectedSample];

  m_view->ShowNonFoundResults(context.m_nonFoundResults,
                              context.m_nonFoundResultsEdits.GetEntries());
  m_view->ShowMarks(context);
  m_view->OnResultChanged(m_selectedSample, View::ResultType::Found,
                          ResultsEdits::Update::MakeAll());
  m_view->OnResultChanged(m_selectedSample, View::ResultType::NonFound,
                          ResultsEdits::Update::MakeAll());
  m_view->OnSampleChanged(m_selectedSample, context.IsUseless(), context.HasChanges());
  m_view->OnSamplesChanged(m_contexts.HasChanges());

  m_view->SetResultsEdits(m_selectedSample, context.m_foundResultsEdits,
                          context.m_nonFoundResultsEdits);
  m_view->OnSearchCompleted();
}

void MainModel::OnChangeAllRelevancesClicked(ResultsEdits::Relevance relevance)
{
  CHECK_GREATER_OR_EQUAL(m_selectedSample, 0, ());
  CHECK_LESS(static_cast<size_t>(m_selectedSample), m_contexts.Size(), ());
  auto & context = m_contexts[m_selectedSample];

  context.m_foundResultsEdits.SetAllRelevances(relevance);
  context.m_nonFoundResultsEdits.SetAllRelevances(relevance);

  m_view->OnResultChanged(m_selectedSample, View::ResultType::Found,
                          ResultsEdits::Update::MakeAll());
  m_view->OnResultChanged(m_selectedSample, View::ResultType::NonFound,
                          ResultsEdits::Update::MakeAll());
  m_view->OnSampleChanged(m_selectedSample, context.IsUseless(), context.HasChanges());
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

  auto ft = m_loader.Load(id);
  CHECK(ft, ("Can't load feature:", id));
  search::Matcher matcher(m_loader);

  auto const & nonFoundResults = context.m_nonFoundResults;
  CHECK_EQUAL(nonFoundResults.size(), context.m_nonFoundResultsEdits.NumEntries(), ());
  for (size_t i = 0; i < nonFoundResults.size(); ++i)
  {
    auto const & result = context.m_nonFoundResults[i];
    if (matcher.Matches(context.m_sample.m_query, result, *ft))
      return fn(context.m_nonFoundResultsEdits, i);
  }
}
