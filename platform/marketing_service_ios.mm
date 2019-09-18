#include "platform/marketing_service.hpp"

void MarketingService::SendPushWooshTag(std::string const & tag)
{
  SendPushWooshTag(tag, std::vector<std::string>{"1"});
}

void MarketingService::SendPushWooshTag(std::string const & tag, std::string const & value)
{
  SendPushWooshTag(tag, std::vector<std::string>{value});
}

void MarketingService::SendPushWooshTag(std::string const & tag, std::vector<std::string> const & values)
{
  if (m_pushwooshSender)
    m_pushwooshSender(tag, values);
}

void MarketingService::SendMarketingEvent(std::string const & tag, std::map<std::string, std::string> const & params)
{
  if (m_marketingSender)
    m_marketingSender(tag, params);
}
