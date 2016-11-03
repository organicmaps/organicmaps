#include "platform/marketing_service.hpp"

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
  if (m_pushwooshSender)
    m_pushwooshSender(tag, values);
}

void MarketingService::SendMarketingEvent(string const & tag, map<string, string> const & params)
{
  if (m_marketingSender)
    m_marketingSender(tag, params);
}
