#pragma once

#include "search/hotels_classifier.hpp"
#include "search/params.hpp"

#include "std/function.hpp"

namespace search
{
class Results;

// An on-results-callback that should be used for interactive search.
//
// *NOTE* the class is NOT thread safe.
class InteractiveSearchCallback
{
public:
  using TSetDisplacementMode = function<void()>;
  using TOnResults = search::TOnResults;

  InteractiveSearchCallback(TSetDisplacementMode && setMode, TOnResults && onResults);

  void operator()(search::Results const & results);

private:
  TSetDisplacementMode m_setMode;
  TOnResults m_onResults;

  search::HotelsClassifier m_hotelsClassif;
  bool m_hotelsModeSet;
};
}  // namespace search
