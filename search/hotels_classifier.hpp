#pragma once

#include <cstdint>
#include <vector>

namespace search
{
class Result;
class Results;

// A binary classifier that can be used in conjunction with search
// engine to decide whether the query is related to hotel search.
//
// Two ways we use to decide whether the user was searching for hotels:
// 1. We may infer it from the query text.
// 2. We may infer it from the query results: if the majority are hotels,
//    probably the query was too.
//
// *NOTE* This class is NOT thread safe.
class HotelsClassifier
{
public:
  void Add(Result const & result);

  // The types are parsed from the original query in search::Processor.
  void PrecheckHotelQuery(std::vector<uint32_t> const & types);

  void Clear();

  bool IsHotelResults() const;

private:
  uint64_t m_numHotels = 0;
  uint64_t m_numResults = 0;
  bool m_looksLikeHotelQuery = false;
};
}  // namespace search
