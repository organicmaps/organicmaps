#pragma once
#include "context.hpp"
#include "edits.hpp"

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class ResultsEdits;
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
  virtual void OnSearchStarted() = 0;
  virtual void OnSearchCompleted() = 0;
  virtual void ShowSample(size_t index, search::Sample const & sample, std::optional<m2::PointD> const & position,
                          bool isUseless, bool hasEdits) = 0;

  virtual void AddFoundResults(search::Results const & results) = 0;
  virtual void ShowNonFoundResults(std::vector<search::Sample::Result> const & results,
                                   std::vector<ResultsEdits::Entry> const & entries) = 0;

  virtual void ShowMarks(Context const & context) = 0;

  virtual void MoveViewportToResult(search::Result const & result) = 0;
  virtual void MoveViewportToResult(search::Sample::Result const & result) = 0;
  virtual void MoveViewportToRect(m2::RectD const & rect) = 0;

  virtual void OnResultChanged(size_t sampleIndex, ResultType type, ResultsEdits::Update const & update) = 0;
  virtual void SetResultsEdits(size_t index, ResultsEdits & foundResultsEdits, ResultsEdits & nonFoundResultsEdits) = 0;
  virtual void OnSampleChanged(size_t sampleIndex, bool isUseless, bool hasEdits) = 0;
  virtual void OnSamplesChanged(bool hasEdits) = 0;

  virtual void ShowError(std::string const & msg) = 0;

  virtual void Clear() = 0;

protected:
  Model * m_model = nullptr;
};

inline std::string DebugPrint(View::ResultType type)
{
  switch (type)
  {
  case View::ResultType::Found: return "Found";
  case View::ResultType::NonFound: return "NonFound";
  }
  return "Unknown";
}
