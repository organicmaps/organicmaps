#pragma once

#include "platform/http_request.hpp"

#include "std/chrono.hpp"
#include "std/function.hpp"
#include "std/initializer_list.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

class BookingApi
{
  string m_affiliateId;
  string m_apiUrl;
  bool m_testingMode;

public:
  struct HotelPhotoUrls
  {
    string m_small;
    string m_original;
  };

  struct HotelReview
  {
    HotelReview() = default;
    // C++11 doesn't allow aggragate initialization for structs with default member initializer.
    // But C++14 does.
    HotelReview(string const & reviewPositive,
                string const & reviewNegative,
                string const & reviewNeutral,
                string const & author,
                string const & authorPictUrl,
                float const rating,
                time_point<system_clock> const date)
      : m_reviewPositive(reviewPositive)
      , m_reviewNegative(reviewNegative)
      , m_reviewNeutral(reviewNeutral)
      , m_author(author)
      , m_authorPictUrl(authorPictUrl)
      , m_rating(rating)
      , m_date(date)
    {
    }


    static HotelReview CriticReview(string const & reviewPositive,
                                    string const & reviewNegative,
                                    string const & author,
                                    string const & authorPictUrl,
                                    float const rating,
                                    time_point<system_clock> const date)
    {
      return {
        reviewPositive,
        reviewNegative,
        "",
        author,
        authorPictUrl,
        rating,
        date
     };
    }

    static HotelReview NeutralReview(string const & reviewNeutral,
                                     string const & author,
                                     string const & authorPictUrl,
                                     float const rating,
                                     time_point<system_clock> const date)
    {
      return {
        "",
        "",
        reviewNeutral,
        author,
        authorPictUrl,
        rating,
        date
     };
    }

    static auto constexpr kInvalidRating = numeric_limits<float>::max();

    /// Review text. There can be either one or both positive/negative review or
    /// a neutral one.
    string m_reviewPositive;
    string m_reviewNegative;
    string m_reviewNeutral;
    /// Review author name.
    string m_author;
    /// Url to a author's picture.
    string m_authorPictUrl;
    /// Author's hotel evaluation.
    float m_rating = kInvalidRating;
    /// An issue date.
    time_point<system_clock> m_date;
  };

  struct Facility
  {
    string m_id;
    string m_localizedName;
  };

  struct HotelInfo
  {
    string m_hotelId;

    string m_description;
    vector<HotelPhotoUrls> m_photos;
    vector<Facility> m_facilities;
    vector<HotelReview> m_reviews;
  };

  static constexpr const char kDefaultCurrency[1] = {0};

  BookingApi();
  string GetBookHotelUrl(string const & baseUrl, string const & lang = string()) const;
  string GetDescriptionUrl(string const & baseUrl, string const & lang = string()) const;
  inline void SetTestingMode(bool testing) { m_testingMode = testing; }

  // Real-time information methods (used for retriving rapidly changing information).
  // These methods send requests directly to Booking.
  void GetMinPrice(string const & hotelId, string const & currency,
                   function<void(string const &, string const &)> const & fn);


  // Static information methods (use for information that can be cached).
  // These methods use caching server to prevent Booking from being ddossed.
  void GetHotelInfo(string const & hotelId, string const & lang,
                    function<void(HotelInfo const & hotelInfo)> const & fn);

protected:
  unique_ptr<downloader::HttpRequest> m_request;
  string MakeApiUrl(string const & func, initializer_list<pair<string, string>> const & params);
};
