#include "ugc/api.hpp"

#include "platform/platform.hpp"

#include <chrono>

using namespace std;
using namespace ugc;

namespace ugc
{
namespace
{
Time FromDaysAgo(Time time, uint32_t days)
{
  return time - std::chrono::hours(days * 24);
}
}  // namespace

Api::Api(Index const & index, std::string const & filename) : m_index(index), m_storage(filename) {}

void Api::GetUGC(FeatureID const & id, UGCCallback callback)
{
  m_thread.Push([=] { GetUGCImpl(id, callback); });
}

void Api::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  m_thread.Push([=] { SetUGCUpdate(id, ugc); });
}

// static
UGC Api::MakeTestUGC1(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 4.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  Reviews reviews;
  reviews.emplace_back(20 /* id */, Text("Damn good coffee", StringUtf8Multilang::kEnglishCode),
                       Author("Cole"),
                       5.0 /* rating */, FromDaysAgo(now, 10));
  reviews.emplace_back(
      67812 /* id */, Text("Clean place, reasonably priced", StringUtf8Multilang::kDefaultCode),
      Author("Cooper"), 5.0 /* rating */, FromDaysAgo(now, 1));

  return UGC(records, reviews, 4.5 /* rating */, 4000000000 /* votes */);
}

// static
UGC Api::MakeTestUGC2(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 5.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  vector<Review> reviews;
  reviews.emplace_back(
      119 /* id */, Text("This pie's so good it is a crime", StringUtf8Multilang::kDefaultCode),
      Author("Cooper"), 5.0 /* rating */, FromDaysAgo(now, 1));

  return UGC(records, reviews, 5.0 /* rating */, 0 /* votes */);
}

// static
UGCUpdate Api::MakeTestUGCUpdate(Time now)
{
  Ratings records;
  records.emplace_back("food" /* key */, 4.0 /* value */);
  records.emplace_back("service" /* key */, 5.0 /* value */);
  records.emplace_back("music" /* key */, 5.0 /* value */);

  Text text{"It's aways nice to visit this place", StringUtf8Multilang::kEnglishCode};

  Time time{FromDaysAgo(now, 1)};

  return UGCUpdate(records, text, time);
}

void Api::GetUGCImpl(FeatureID const & id, UGCCallback callback)
{
  // TODO (@y, @mgsergio): retrieve static UGC
  UGC ugc;
  UGCUpdate update;

  if (!id.IsValid())
  {
    GetPlatform().RunOnGuiThread([ugc, update, callback] { callback(ugc, update); });
    return;
  }

  ugc = MakeTestUGC1();
  GetPlatform().RunOnGuiThread([ugc, update, callback] { callback(ugc, update); });
}

void Api::SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc)
{
  m_storage.SetUGCUpdate(id, ugc);
}
}  // namespace ugc
