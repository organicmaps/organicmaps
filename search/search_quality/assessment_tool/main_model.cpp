#include "search/search_quality/assessment_tool/main_model.hpp"

#include "search/search_quality/assessment_tool/view.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

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
  m_view->ShowSample(m_samples[index]);
}
