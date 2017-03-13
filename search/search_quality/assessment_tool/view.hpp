#pragma once

#include "search/search_quality/sample.hpp"

#include <string>
#include <vector>

class Model;

class View
{
public:
  virtual ~View() = default;

  void SetModel(Model & model) { m_model = &model; }

  virtual void SetSamples(std::vector<search::Sample> const & samples) = 0;
  virtual void ShowSample(search::Sample const & sample) = 0;
  virtual void ShowError(std::string const & msg) = 0;

protected:
  Model * m_model = nullptr;
};
