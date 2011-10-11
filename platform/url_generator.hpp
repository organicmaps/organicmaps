#pragma once

#include "../base/pseudo_random.hpp"
#include "../base/timer.hpp"

#include "../std/vector.hpp"
#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"

class UrlGenerator
{
  LCG32 m_randomGenerator;
  vector<string> m_firstGroup;
  vector<string> m_secondGroup;

  /// For moving average speed calculations
  my::Timer m_timer;
  /// Stores time in seconds from start and downloaded amount at that moment
  typedef pair<double, int64_t> MarkT;
  list<MarkT> m_speeds;
  double m_lastSecond;

public:
  UrlGenerator();
  explicit UrlGenerator(vector<string> const & firstGroup, vector<string> const & secondGroup);

  /// @return Always return empty string if all urls were already popped
  string PopNextUrl();

  /// @return -1 means speed is unknown
  void UpdateSpeed(int64_t totalBytesRead);
  int64_t CurrentSpeed() const;
  string GetFasterUrl();
};
