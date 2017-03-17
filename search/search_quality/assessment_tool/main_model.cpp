#include "search/search_quality/assessment_tool/main_model.hpp"

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
#include <iterator>

MainModel::MainModel(Framework & framework)
  : m_framework(framework), m_index(m_framework.GetIndex())
{
}

void MainModel::Open(std::string const & path)
{
  CHECK(m_view, ());

  std::ifstream ifs(path);
  if (!ifs)
  {
    m_view->ShowError("Can't open file: " + path);
    return;
  }

  std::string const contents((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));
  std::vector<search::Sample> samples;
  if (!search::Sample::DeserializeFromJSON(contents, samples))
  {
    m_view->ShowError("Can't parse samples: " + path);
    return;
  }

  InvalidateSearch();

  m_samples.swap(samples);
  m_view->SetSamples(m_samples);
}

void MainModel::OnSampleSelected(int index)
{
  CHECK_GREATER_OR_EQUAL(index, 0, ());
  CHECK_LESS(index, m_samples.size(), ());
  CHECK(m_view, ());

  auto const & sample = m_samples[index];
  m_view->ShowSample(m_samples[index]);

  InvalidateSearch();

  auto & engine = m_framework.GetSearchEngine();
  {
    auto latLon = MercatorBounds::ToLatLon(sample.m_pos);

    search::SearchParams params;
    params.m_query = strings::ToUtf8(sample.m_query);
    params.m_inputLocale = sample.m_locale;
    params.m_suggestsEnabled = false;
    params.SetPosition(latLon.lat, latLon.lon);

    auto const timestamp = m_queryTimestamp;
    m_numShownResults = 0;

    params.m_onResults = [this, sample, timestamp](search::Results const & results) {
      vector<search::Sample::Result::Relevance> relevances;
      if (results.IsEndedNormal())
      {
        search::Matcher matcher(m_index);

        vector<search::Result> const actual(results.begin(), results.end());
        std::vector<size_t> goldenMatching;
        {
          std::vector<size_t> actualMatching;
          matcher.Match(sample.m_results, actual, goldenMatching, actualMatching);
        }

        relevances.assign(actual.size(), search::Sample::Result::RELEVANCE_IRRELEVANT);
        for (size_t i = 0; i < goldenMatching.size(); ++i) {
          if (goldenMatching[i] != search::Matcher::kInvalidId)
            relevances[goldenMatching[i]] = sample.m_results[i].m_relevance;
        }
      }

      GetPlatform().RunOnGuiThread(
          [this, timestamp, results, relevances]() { OnResults(timestamp, results, relevances); });
    };

    m_queryHandle = engine.Search(params, sample.m_viewport);
  }
}

void MainModel::OnResults(uint64_t timestamp, search::Results const & results,
                          vector<search::Sample::Result::Relevance> const & relevances)
{
  if (timestamp != m_queryTimestamp)
    return;
  CHECK_LESS_OR_EQUAL(m_numShownResults, results.GetCount(), ());
  m_view->ShowResults(results.begin() + m_numShownResults, results.end());
  m_numShownResults = results.GetCount();

  if (results.IsEndedNormal())
    m_view->SetResultRelevances(relevances);
}

void MainModel::InvalidateSearch()
{
  ++m_queryTimestamp;
  if (auto handle = m_queryHandle.lock())
    handle->Cancel();
}
