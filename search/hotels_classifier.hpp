#pragma once

#include <cstdint>

namespace search
{
class Result;
class Results;
// A binary classifier that can be used in conjunction with search
// engine to decide whether the majority of results are hotels or not.
//
// *NOTE* This class is NOT thread safe.
class HotelsClassifier
{
public:
  static bool IsHotelResults(Results const & results);

  void Add(Result const & result);
  void Clear();

  bool IsHotelResults() const;

private:
  uint64_t m_numHotels = 0;
  uint64_t m_numResults = 0;
};
}  // namespace search
