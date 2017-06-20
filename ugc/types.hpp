#pragma once

#include "indexer/feature_decl.hpp"

#include "coding/hex.hpp"


#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string>
#include <vector>

namespace ugc
{
using TranslationKey = std::string;
using Time = std::chrono::time_point<std::chrono::system_clock>;

enum class Sentiment
{
  Positive,
  Negative
};

inline std::string DebugPrint(Sentiment const & sentiment)
{
  switch (sentiment)
  {
  case Sentiment::Positive: return "Positive";
  case Sentiment::Negative: return "Negative";
  }
}

struct RatingRecord
{
  RatingRecord() = default;
  RatingRecord(TranslationKey const & key, float const value) : m_key(key), m_value(value) {}

  bool operator==(RatingRecord const & rhs) const
  {
    return m_key == rhs.m_key && m_value == rhs.m_value;
  }

  friend std::string DebugPrint(RatingRecord const & ratingRecord)
  {
    std::ostringstream os;
    os << "RatingRecord [ " << ratingRecord.m_key << " " << ratingRecord.m_value << " ]";
    return os.str();
  }

  TranslationKey m_key{};
  float m_value{};
};

struct Rating
{
  Rating() = default;
  Rating(std::vector<RatingRecord> const & ratings, float const aggValue)
    : m_ratings(ratings), m_aggValue(aggValue)
  {
  }

  bool operator==(Rating const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_aggValue == rhs.m_aggValue;
  }

  friend std::string DebugPrint(Rating const & rating)
  {
    std::ostringstream os;
    os << "Rating [ ratings:" << ::DebugPrint(rating.m_ratings) << ", aggValue:" << rating.m_aggValue
       << " ]";
    return os.str();
  }

  std::vector<RatingRecord> m_ratings;
  float m_aggValue{};
};

struct UID
{
  UID() = default;
  UID(uint64_t const hi, uint64_t const lo) : m_hi(hi), m_lo(lo) {}

  std::string ToString() const { return NumToHex(m_hi) + NumToHex(m_lo); }

  bool operator==(UID const & rhs) const { return m_hi == rhs.m_hi && m_lo == rhs.m_lo; }

  friend std::string DebugPrint(UID const & uid)
  {
    std::ostringstream os;
    os << "UID [ " << uid.ToString() << " ]";
    return os.str();
  }

  uint64_t m_hi{};
  uint64_t m_lo{};
};

struct Author
{
  Author() = default;
  Author(UID const & uid, std::string const & name) : m_uid(uid), m_name(name) {}

  bool operator==(Author const & rhs) const { return m_uid == rhs.m_uid && m_name == rhs.m_name; }

  friend std::string DebugPrint(Author const & author)
  {
    std::ostringstream os;
    os << "Author [ " << DebugPrint(author.m_uid) << " " << author.m_name << " ]";
    return os.str();
  }

  UID m_uid{};
  std::string m_name;
};

struct Text
{
  Text() = default;
  Text(std::string const & text, uint8_t const lang) : m_text(text), m_lang(lang) {}

  bool operator==(Text const & rhs) const { return m_lang == rhs.m_lang && m_text == rhs.m_text; }

  friend std::string DebugPrint(Text const & text)
  {
    std::ostringstream os;
    os << "Text [ " << StringUtf8Multilang::GetLangByCode(text.m_lang) << ": " << text.m_text
       << " ]";
    return os.str();
  }

  std::string m_text;
  uint8_t m_lang = StringUtf8Multilang::kDefaultCode;
};

struct Review
{
  using ReviewId = uint32_t;

  Review() = default;
  Review(ReviewId id, Text const & text, Author const & author, float const rating,
         Sentiment const sentiment, Time const & time)
    : m_text(text), m_author(author), m_rating(rating), m_sentiment(sentiment), m_time(time)
  {
  }

  bool operator==(Review const & rhs) const
  {
    return m_id == rhs.m_id && m_text == rhs.m_text && m_author == rhs.m_author &&
           m_rating == rhs.m_rating && m_sentiment == rhs.m_sentiment && m_time == rhs.m_time;
  }

  uint32_t DaysSinceEpoch() const
  {
    auto const hours = std::chrono::duration_cast<std::chrono::hours>(m_time.time_since_epoch());
    return static_cast<uint32_t>(hours.count()) / 24;
  }

  void SetDaysSinceEpoch(uint32_t days)
  {
    auto const hours = std::chrono::hours(days * 24);
    m_time = Time(hours);
  }

  friend std::string DebugPrint(Review const & review)
  {
    std::ostringstream os;
    os << "Review [ ";
    os << "id:" << review.m_id << ", ";
    os << "text:" << DebugPrint(review.m_text) << ", ";
    os << "author:" << DebugPrint(review.m_author) << ", ";
    os << "rating:" << review.m_rating << ", ";
    os << "sentiment:" << DebugPrint(review.m_sentiment) << ", ";
    os << "days since epoch:" << review.DaysSinceEpoch() << " ]";
    return os.str();
  }

  ReviewId m_id{};
  Text m_text{};
  Author m_author{};
  // A rating of a review itself. It is accumulated from other users
  // likes or dislikes.
  float m_rating{};
  // A positive/negative sentiment given to a place by an author.
  Sentiment m_sentiment = Sentiment::Positive;
  Time m_time{};
};

struct Attribute
{
  Attribute() = default;
  Attribute(TranslationKey const & key, TranslationKey const & value) : m_key(key), m_value(value)
  {
  }

  bool operator==(Attribute const & rhs) const
  {
    return m_key == rhs.m_key && m_value == rhs.m_value;
  }

  friend std::string DebugPrint(Attribute const & attribute)
  {
    std::ostringstream os;
    os << "Attribute [ key:" << attribute.m_key << ", value:" << attribute.m_value << " ]";
    return os.str();
  }

  TranslationKey m_key{};
  TranslationKey m_value{};
};

struct UGC
{
  UGC() = default;
  UGC(Rating const & rating, std::vector<Review> const & reviews,
      std::vector<Attribute> const & attributes)
    : m_rating(rating), m_reviews(reviews), m_attributes(attributes)
  {
  }

  bool operator==(UGC const & rhs) const {
    return m_rating == rhs.m_rating && m_reviews == rhs.m_reviews &&
           m_attributes == rhs.m_attributes;
  }

  friend std::string DebugPrint(UGC const & ugc)
  {
    std::ostringstream os;
    os << "UGC [ ";
    os << "rating:" << DebugPrint(ugc.m_rating) << ", ";
    os << "reviews:" << ::DebugPrint(ugc.m_reviews) << ", ";
    os << "attributes:" << ::DebugPrint(ugc.m_attributes) << " ]";
    return os.str();
  }

  Rating m_rating{};
  std::vector<Review> m_reviews;
  std::vector<Attribute> m_attributes;
};

struct ReviewFeedback
{
  ReviewFeedback(Sentiment const sentiment, Time const & time)
    : m_sentiment(sentiment), m_time(time)
  {
  }

  Sentiment m_sentiment;
  Time m_time;
};

struct ReviewAbuse
{
  ReviewAbuse(std::string const & reason, Time const & time) : m_reason(reason), m_time(time) {}

  std::string m_reason;
  Time m_time;
};

struct UGCUpdate
{
  UGCUpdate(Rating ratings, Attribute attribute, ReviewAbuse abuses, ReviewFeedback feedbacks)
    : m_ratings(ratings), m_attribute(attribute), m_abuses(abuses), m_feedbacks(feedbacks)
  {
  }

  Rating m_ratings;
  Attribute m_attribute;

  ReviewAbuse m_abuses;
  ReviewFeedback m_feedbacks;
};
}  // namespace ugc
