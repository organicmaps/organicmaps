#include "ugc/ugc_tests/utils.hpp"

namespace
{
ugc::Time FromDaysAgo(ugc::Time time, uint32_t days)
{
  return time - std::chrono::hours(days * 24);
}
}  // namespace

namespace ugc
{
UGC MakeTestUGC1(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 4.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  Reviews reviews;
  reviews.emplace_back(20 /* id */, Text("Damn good coffee", StringUtf8Multilang::kEnglishCode),
                       Author(UID(987654321 /* hi */, 123456789 /* lo */), "Cole"),
                       5.0 /* rating */, FromDaysAgo(now, 10));
  reviews.emplace_back(
      67812 /* id */, Text("Clean place, reasonably priced", StringUtf8Multilang::kDefaultCode),
      Author(UID(0 /* hi */, 315 /* lo */), "Cooper"), 5.0 /* rating */, FromDaysAgo(now, 1));

  return UGC(records, reviews, 4.5 /* rating */);
}

UGC MakeTestUGC2(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 5.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  vector<Review> reviews;
  reviews.emplace_back(
      119 /* id */, Text("This pie's so good it is a crime", StringUtf8Multilang::kDefaultCode),
      Author(UID(0 /* hi */, 315 /* lo */), "Cooper"), 5.0 /* rating */, FromDaysAgo(now, 1));

  return UGC(records, reviews, 5.0 /* rating */);
}

UGCUpdate MakeTestUGCUpdate(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 4.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  Text text{"It's aways nice to visit this place", StringUtf8Multilang::kEnglishCode};

  Time time{FromDaysAgo(now, 1)};

  return UGCUpdate(records, text, time);
}
}
