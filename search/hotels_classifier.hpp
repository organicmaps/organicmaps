#pragma once

#include "std/cstdint.hpp"

namespace search
{
class Results;

// A binary classifier, that can be used in conjunction with search
// engine to decide whether the majority of results are hotels or not.
//
// *NOTE* This class is NOT thread safe.
class HotelsClassifier
{
public:
  void AddBatch(Results const & results);

  bool IsHotelQuery() const;

private:
  uint64_t m_numHotels = 0;
  uint64_t m_numResults = 0;
};
}  // namespace search
