#include "search/search_quality/assessment_tool/main_model.hpp"

#include "search/search_quality/assessment_tool/view.hpp"

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

MainModel::MainModel(Framework & framework) : m_framework(framework) {}

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

  auto & engine = m_framework.GetSearchEngine();
  {
    auto latLon = MercatorBounds::ToLatLon(sample.m_pos);

    search::SearchParams params;
    params.m_query = strings::ToUtf8(sample.m_query);
    params.m_inputLocale = sample.m_locale;
    params.m_suggestsEnabled = false;
    params.SetPosition(latLon.lat, latLon.lon);

    auto const timestamp = ++m_queryTimestamp;
    m_numShownResults = 0;

    params.m_onResults = [this, timestamp](search::Results const & results) {
      GetPlatform().RunOnGuiThread([this, timestamp, results]() { OnResults(timestamp, results); });
    };

    if (auto handle = m_queryHandle.lock())
      handle->Cancel();
    m_queryHandle = engine.Search(params, sample.m_viewport);
  }
}

void MainModel::OnResults(uint64_t timestamp, search::Results const & results)
{
  if (timestamp != m_queryTimestamp)
    return;
  CHECK_LESS_OR_EQUAL(m_numShownResults, results.GetCount(), ());
  m_view->ShowResults(results.begin() + m_numShownResults, results.end());
  m_numShownResults = results.GetCount();
}
