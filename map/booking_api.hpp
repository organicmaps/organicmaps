#pragma once

#include "std/string.hpp"

class BookingApi
{
  string m_affiliateId;

public:
  BookingApi();
  string GetBookingUrl(string const &baseUrl, string const & lang = string()) const;
  string GetDescriptionUrl(string const &baseUrl, string const & lang = string()) const;
};

