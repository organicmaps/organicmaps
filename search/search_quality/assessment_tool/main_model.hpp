#pragma once

#include "search/search_quality/assessment_tool/model.hpp"
#include "search/search_quality/sample.hpp"

#include <vector>

class Framework;

class MainModel : public Model
{
public:
  // Model overrides:
  void Open(std::string const & path) override;
  void OnSampleSelected(int index) override;

private:
  std::vector<search::Sample> m_samples;
};
