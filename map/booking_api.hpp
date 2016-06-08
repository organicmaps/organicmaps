#pragma once

#include "std/function.hpp"
#include "std/initializer_list.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

class BookingApi
{
  string m_affiliateId;
  string m_apiUrl;

public:
  static constexpr const char kDefaultCurrency[1] = {0};

  BookingApi();
  string GetBookingUrl(string const & baseUrl, string const & lang = string()) const;
  string GetDescriptionUrl(string const & baseUrl, string const & lang = string()) const;
  void GetMinPrice(string const & hotelId, string const & currency,
                   function<void(string const &, string const &)> const & fn);

protected:
  string MakeApiUrl(string const & func, initializer_list<pair<string, string>> const & params);
};
