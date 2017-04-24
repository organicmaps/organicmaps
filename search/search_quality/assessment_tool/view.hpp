#pragma once

#include "search/result.hpp"
#include "search/search_quality/assessment_tool/context.hpp"
#include "search/search_quality/assessment_tool/edits.hpp"
#include "search/search_quality/sample.hpp"

#include "geometry/rect2d.hpp"

#include <cstddef>
#include <string>
#include <vector>

class Edits;
class Model;

class View
{
public:
  enum class ResultType
  {
    Found,
    NonFound
  };

  virtual ~View() = default;

  void SetModel(Model & model) { m_model = &model; }

  virtual void SetSamples(ContextList::SamplesSlice const & samples) = 0;
  virtual void ShowSample(size_t index, search::Sample const & sample, bool positionAvailable,
                          bool hasEdits) = 0;
  virtual void ShowFoundResults(search::Results::ConstIter begin,
                                search::Results::ConstIter end) = 0;
  virtual void ShowNonFoundResults(std::vector<search::Sample::Result> const & results) = 0;

  virtual void MoveViewportToResult(search::Result const & result) = 0;
  virtual void MoveViewportToResult(search::Sample::Result const & result) = 0;
  virtual void MoveViewportToRect(m2::RectD const & rect) = 0;

  virtual void OnResultChanged(size_t sampleIndex, ResultType type,
                               Edits::Update const & update) = 0;
  virtual void EnableSampleEditing(size_t index, Edits & foundResultsEdits,
                                   Edits & nonFoundResultsEdits) = 0;
  virtual void OnSampleChanged(size_t sampleIndex, bool hasEdits) = 0;
  virtual void OnSamplesChanged(bool hasEdits) = 0;

  virtual void ShowError(std::string const & msg) = 0;

  virtual void Clear() = 0;

protected:
  Model * m_model = nullptr;
};
