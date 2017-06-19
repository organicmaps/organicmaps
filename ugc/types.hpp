#pragma once

#include "indexer/feature_decl.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ugc
{
struct RatingRecord
{
  RatingRecord(std::string const & key, float const value)
    : m_key(key)
    , m_value(value)
  {
  }

  std::string m_key;
  float m_value;
};

struct Rating
{
  Rating(std::vector<RatingRecord> const & ratings, float const aggValue)
    : m_ratings(ratings)
    , m_aggValue(aggValue)
  {
  }

  std::vector<RatingRecord> m_ratings;
  float m_aggValue;
};

class UID
{
public:
  UID(uint64_t const hi, uint64_t const lo)
    : m_hi(hi)
    , m_lo(lo)
  {
  }

  std::string ToString() const;

private:
  uint64_t m_hi;
  uint64_t m_lo;
};

struct Author
{
  Author(UID const & uid, std::string const & name)
    : m_uid(uid)
    , m_name(name)
  {
  }

  UID m_uid;
  std::string m_name;
};

struct Text
{
  Text(std::string const & text, uint8_t const lang)
    : m_text(text)
    , m_lang(lang)
  {
  }

  std::string m_text;
  uint8_t m_lang;
};

struct Review
{
  using ReviewId = uint32_t;

  Review(Text const & text, Author const & author,
         float const rating, bool const evaluation,
         std::chrono::time_point<std::chrono::system_clock> const & time)
    : m_text(text)
    , m_author(author)
    , m_rating(rating)
    , m_evaluation(evaluation)
    , m_time(time)
  {
  }

  ReviewId m_id;

  Text m_text;
  Author m_author;
  // A rating of a review itself. It is accumulated from other users likes or dislakes.
  float m_rating;
  // A positive/negative evaluation given to a place by a user.
  bool m_evaluation;
  std::chrono::time_point<std::chrono::system_clock> m_time;
};

struct Attribute
{
  Attribute(std::string const & key, std::string const & value)
    : m_key(key)
    , m_value(value)
  {
  }

  std::string m_key;
  std::string m_value;
};

// struct Media
// {
//   std::unique_ptr<MediaImpl> m_data;
// };

struct UGC
{
  UGC(Rating const & rating,
      std::vector<Review> const & reviews,
      std::vector<Attribute> const & attributes)
    : m_rating(rating)
    , m_reviews(reviews)
    , m_attributes(attributes)
  {
  }

  Rating m_rating;
  std::vector<Review> m_reviews;
  std::vector<Attribute> m_attributes;
  // Media m_media;
};

struct ReviewFeedback
{
  ReviewFeedback(bool const evaluation,
                 std::chrono::time_point<std::chrono::system_clock> const time)
    : m_evaluation(evaluation)
    , m_time(time)
  {
  }

  bool m_evaluation;
  std::chrono::time_point<std::chrono::system_clock> m_time;
};

struct ReviewAbuse
{
  ReviewAbuse(std::string const & reason,
              std::chrono::time_point<std::chrono::system_clock> const & time)
    : m_reason(reason)
    , m_time(time)
  {
  }

  std::string m_reason;
  std::chrono::time_point<std::chrono::system_clock> m_time;
};

struct UGCUpdate
{
  UGCUpdate(Rating ratings, Attribute attribute,
            ReviewAbuse abuses, ReviewFeedback feedbacks)
    : m_ratings(ratings)
    , m_attribute(attribute)
    , m_abuses(abuses)
    , m_feedbacks(feedbacks)
  {
  }

  Rating m_ratings;
  Attribute m_attribute;

  ReviewAbuse m_abuses;
  ReviewFeedback m_feedbacks;
};
}  // namespace ugc
