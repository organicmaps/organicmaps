#pragma once

#include "search/engine.hpp"
#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/sample.hpp"

#include <cstdint>
#include <vector>
#include <memory>

class Framework;

namespace search
{
class Results;
}

class MainModel : public Model
{
public:
  explicit MainModel(Framework & framework);

  // Model overrides:
  void Open(std::string const & path) override;
  void OnSampleSelected(int index) override;

private:
  void OnResults(uint64_t timestamp, search::Results const & results);

  Framework & m_framework;
  std::vector<search::Sample> m_samples;

  std::weak_ptr<search::ProcessorHandle> m_queryHandle;
  uint64_t m_queryTimestamp = 0;
  size_t m_numShownResults = 0;
};
