#include "platform/marketing_service.hpp"

#include "Platform.hpp"

using namespace std;

void MarketingService::SendPushWooshTag(string const & tag)
{
  SendPushWooshTag(tag, vector<string>{"1"});
}

void MarketingService::SendPushWooshTag(string const & tag, string const & value)
{
  SendPushWooshTag(tag, vector<string>{value});
}

void MarketingService::SendPushWooshTag(string const & tag, vector<string> const & values)
{
  android::Platform::Instance().SendPushWooshTag(tag, values);
}

void MarketingService::SendMarketingEvent(string const & tag, map<string, string> const & params)
{
  android::Platform::Instance().SendMarketingEvent(tag, params);
}
