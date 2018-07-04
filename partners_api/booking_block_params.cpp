#include "partners_api/booking_block_params.hpp"

#include "base/string_utils.hpp"

using namespace base;

namespace booking
{
// static
BlockParams BlockParams::MakeDefault()
{
  BlockParams result;
  // Use tomorrow and day after tomorrow by default.
  result.m_checkin = Clock::now();
  result.m_checkout = Clock::now() + std::chrono::hours(24);
  // Information about sales by default.
  result.m_extras = {"deal_smart", "deal_lastm", "photos"};

  return result;
}

url::Params BlockParams::Get() const
{
  url::Params params = {{"hotel_ids", m_hotelId},
                        {"checkin", FormatTime(m_checkin)},
                        {"checkout", FormatTime(m_checkout)}};

  if (!m_currency.empty())
    params.emplace_back("currency", m_currency);

  if (!m_extras.empty())
    params.emplace_back("extras", strings::JoinStrings(m_extras, ','));

  if (!m_language.empty())
    params.emplace_back("language", m_language);

  return params;
}

bool BlockParams::IsEmpty() const
{
  return m_checkin == Time() || m_checkout == Time();
}

bool BlockParams::Equals(ParamsBase const & rhs) const
{
  return rhs.Equals(*this);
}

bool BlockParams::Equals(BlockParams const & rhs) const
{
  return m_checkin == rhs.m_checkin && m_checkout == rhs.m_checkout &&
    m_currency == rhs.m_currency && m_extras == rhs.m_extras && m_language == rhs.m_language;
}

void BlockParams::Set(ParamsBase const & src)
{
  src.CopyTo(*this);
}
}  // namepace booking
