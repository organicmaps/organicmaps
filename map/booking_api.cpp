#include "booking_api.hpp"

#include "private.h"

BookingApi::BookingApi() : m_affiliateId(BOOKING_AFFILIATE_ID) {}
string BookingApi::GetBookingUrl(string const & baseUrl, string const & /* lang */) const
{
  return GetDescriptionUrl(baseUrl) + "#availability";
}

string BookingApi::GetDescriptionUrl(string const & baseUrl, string const & /* lang */) const
{
  return baseUrl + "?affiliate_id=" + m_affiliateId;
}
