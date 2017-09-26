#pragma once

#include "indexer/feature_decl.hpp"

#include "coding/hex.hpp"

#include "base/visitor.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string>
#include <vector>

namespace ugc
{
using Clock = std::chrono::system_clock;
using Time = std::chrono::time_point<Clock>;

struct TranslationKey
{
  TranslationKey() = default;
  TranslationKey(std::string const & key): m_key(key) {}
  TranslationKey(char const * key): m_key(key) {}

  DECLARE_VISITOR(visitor(m_key, "key"))

  bool operator==(TranslationKey const & rhs) const { return m_key == rhs.m_key; }
  bool operator<(TranslationKey const & rhs) const { return m_key < rhs.m_key; }

  friend std::string DebugPrint(TranslationKey const & key)
  {
    return "TranslationKey [ " + key.m_key + " ]";
  }

  std::string m_key;
};

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

inline uint32_t ToDaysSinceEpoch(Time const & time)
{
  auto const hours = std::chrono::duration_cast<std::chrono::hours>(time.time_since_epoch());
  return static_cast<uint32_t>(hours.count()) / 24;
}

inline Time FromDaysSinceEpoch(uint32_t days)
{
  auto const hours = std::chrono::hours(days * 24);
  return Time(hours);
}

inline uint32_t DaysAgo(Time const & time)
{
  auto const now = std::chrono::system_clock::now();
  if (now < time)
    return 0;
  auto const h = std::chrono::duration_cast<std::chrono::hours>(now - time).count() / 24;
  return static_cast<uint32_t>(h);
}

struct RatingRecord
{
  RatingRecord() = default;
  RatingRecord(TranslationKey const & key, float const value) : m_key(key), m_value(value) {}

  DECLARE_VISITOR(visitor(m_key, "key"), visitor.VisitRating(m_value, "value"))

  bool operator==(RatingRecord const & rhs) const
  {
    return m_key == rhs.m_key && m_value == rhs.m_value;
  }

  friend std::string DebugPrint(RatingRecord const & ratingRecord)
  {
    std::ostringstream os;
    os << "RatingRecord [ " << DebugPrint(ratingRecord.m_key) << " " << ratingRecord.m_value
       << " ]";
    return os.str();
  }

  TranslationKey m_key{};
  float m_value{};
};

using Ratings = std::vector<RatingRecord>;

struct Rating
{
  Rating() = default;
  Rating(std::vector<RatingRecord> const & ratings, float const aggValue)
    : m_ratings(ratings), m_aggValue(aggValue)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor.VisitRating(m_aggValue, "aggValue"))

  bool operator==(Rating const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_aggValue == rhs.m_aggValue;
  }

  friend std::string DebugPrint(Rating const & rating)
  {
    std::ostringstream os;
    os << "Rating [ ratings:" << ::DebugPrint(rating.m_ratings)
       << ", aggValue:" << rating.m_aggValue << " ]";
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

  DECLARE_VISITOR(visitor(m_hi, "hi"), visitor(m_lo, "lo"));

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

  DECLARE_VISITOR(visitor(m_uid, "uid"), visitor(m_name, "name"));

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

  DECLARE_VISITOR(visitor(m_lang, "lang"), visitor(m_text, "text"));

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
         Time const & time)
    : m_text(text), m_author(author), m_rating(rating), m_time(time)
  {
  }

  DECLARE_VISITOR(visitor(m_id, "id"), visitor(m_text, "text"), visitor(m_author, "author"),
                  visitor.VisitRating(m_rating, "rating"), visitor(m_time, "time"))

  bool operator==(Review const & rhs) const
  {
    return m_id == rhs.m_id && m_text == rhs.m_text && m_author == rhs.m_author &&
           m_rating == rhs.m_rating && m_time == rhs.m_time;
  }

  friend std::string DebugPrint(Review const & review)
  {
    std::ostringstream os;
    os << "Review [ ";
    os << "id:" << review.m_id << ", ";
    os << "text:" << DebugPrint(review.m_text) << ", ";
    os << "author:" << DebugPrint(review.m_author) << ", ";
    os << "rating:" << review.m_rating << ", ";
    os << "days since epoch:" << ToDaysSinceEpoch(review.m_time) << " ]";
    return os.str();
  }

  ReviewId m_id{};
  Text m_text{};
  Author m_author{};
  float m_rating{};
  Time m_time{};
};

using Reviews = std::vector<Review>;

struct Attribute
{
  Attribute() = default;
  Attribute(TranslationKey const & key, TranslationKey const & value) : m_key(key), m_value(value)
  {
  }

  DECLARE_VISITOR(visitor(m_key, "key"), visitor(m_value, "value"))

  bool operator==(Attribute const & rhs) const
  {
    return m_key == rhs.m_key && m_value == rhs.m_value;
  }

  friend std::string DebugPrint(Attribute const & attribute)
  {
    std::ostringstream os;
    os << "Attribute [ key:" << DebugPrint(attribute.m_key)
       << ", value:" << DebugPrint(attribute.m_value) << " ]";
    return os.str();
  }

  TranslationKey m_key{};
  TranslationKey m_value{};
};

struct UGC
{
  UGC() = default;
  UGC(Ratings const & records, Reviews const & reviews, float const rating)
    : m_ratings(records), m_reviews(reviews), m_aggRating(rating)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor(m_reviews, "reviews"),
                  visitor.VisitRating(m_aggRating, "aggRating"))

  bool operator==(UGC const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_reviews == rhs.m_reviews;
  }

  friend std::string DebugPrint(UGC const & ugc)
  {
    std::ostringstream os;
    os << "UGC [ ";
    os << "records:" << ::DebugPrint(ugc.m_ratings) << ", ";
    os << "reviews:" << ::DebugPrint(ugc.m_reviews) << " ]";
    return os.str();
  }

  Ratings m_ratings;
  Reviews m_reviews;
  float m_aggRating{};
};

struct UGCUpdate
{
  UGCUpdate() = default;
  UGCUpdate(Ratings const & records, Text const & text, Time const & time)
    : m_ratings(records), m_text(text), m_time(time)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor(m_text, "text"), visitor(m_time, "time"))

  bool operator==(UGCUpdate const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_text == rhs.m_text && m_time == rhs.m_time;
  }

  friend std::string DebugPrint(UGCUpdate const & ugcUpdate)
  {
    std::ostringstream os;
    os << "UGCUpdate [ ";
    os << "records:" << ::DebugPrint(ugcUpdate.m_ratings) << ", ";
    os << "text:" << DebugPrint(ugcUpdate.m_text) << ", ";
    os << "days since epoch:" << ToDaysSinceEpoch(ugcUpdate.m_time) << " ]";
    return os.str();
  }

  Ratings m_ratings;
  Text m_text;
  Time m_time{};
};

struct ReviewFeedback
{
  ReviewFeedback() = default;
  ReviewFeedback(Sentiment const sentiment, Time const & time)
    : m_sentiment(sentiment), m_time(time)
  {
  }

  Sentiment m_sentiment{};
  Time m_time{};
};

struct ReviewAbuse
{
  ReviewAbuse() = default;
  ReviewAbuse(std::string const & reason, Time const & time) : m_reason(reason), m_time(time) {}

  std::string m_reason{};
  Time m_time{};
};
}  // namespace ugc

