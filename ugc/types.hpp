#pragma once

#include "geometry/point2d.hpp"

#include "indexer/feature_decl.hpp"

#include "coding/hex.hpp"

#include "base/math.hpp"
#include "base/visitor.hpp"

#include "defines.hpp"

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

  DECLARE_VISITOR_AND_DEBUG_PRINT(TranslationKey, visitor(m_key, "key"))

  bool operator==(TranslationKey const & rhs) const { return m_key == rhs.m_key; }
  bool operator<(TranslationKey const & rhs) const { return m_key < rhs.m_key; }

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

inline uint64_t ToMillisecondsSinceEpoch(Time const & time)
{
  auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());
  return static_cast<uint64_t>(millis.count());
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

  TranslationKey m_key{};
  float m_value{};
};

inline std::string DebugPrint(RatingRecord const & ratingRecord)
{
  std::ostringstream os;
  os << "RatingRecord [ " << DebugPrint(ratingRecord.m_key) << " " << ratingRecord.m_value
     << " ]";
  return os.str();
}

using Ratings = std::vector<RatingRecord>;

struct UID
{
  UID() = default;
  UID(uint64_t const hi, uint64_t const lo) : m_hi(hi), m_lo(lo) {}

  std::string ToString() const { return NumToHex(m_hi) + NumToHex(m_lo); }

  DECLARE_VISITOR_AND_DEBUG_PRINT(UID, visitor(m_hi, "hi"), visitor(m_lo, "lo"))

  bool operator==(UID const & rhs) const { return m_hi == rhs.m_hi && m_lo == rhs.m_lo; }

  uint64_t m_hi{};
  uint64_t m_lo{};
};

using Author = std::string;

struct Text
{
  Text() = default;
  Text(std::string const & text, uint8_t const lang) : m_text(text), m_lang(lang) {}

  DECLARE_VISITOR(visitor(m_text, "text"), visitor.VisitLang(m_lang, "lang"))

  bool operator==(Text const & rhs) const { return m_lang == rhs.m_lang && m_text == rhs.m_text; }

  std::string m_text;
  uint8_t m_lang = StringUtf8Multilang::kDefaultCode;
};

inline std::string DebugPrint(Text const & text)
{
  std::ostringstream os;
  os << "Text [ " << StringUtf8Multilang::GetLangByCode(text.m_lang) << ": " << text.m_text
     << " ]";
  return os.str();
}

struct KeyboardText
{
  KeyboardText() = default;
  KeyboardText(std::string const & text, uint8_t const deviceLang,
               std::vector<uint8_t> const & keyboardLangs)
    : m_text(text), m_deviceLang(deviceLang), m_keyboardLangs(keyboardLangs)
  {
  }

  DECLARE_VISITOR(visitor(m_text, "text"), visitor.VisitLang(m_deviceLang, "lang"),
                  visitor.VisitLangs(m_keyboardLangs, "kbd_langs"))

  bool operator==(KeyboardText const & rhs) const
  {
    return m_text == rhs.m_text && m_deviceLang == rhs.m_deviceLang &&
           m_keyboardLangs == rhs.m_keyboardLangs;
  }

  std::string m_text;
  uint8_t m_deviceLang = StringUtf8Multilang::kDefaultCode;
  std::vector<uint8_t> m_keyboardLangs;
};

inline std::string DebugPrint(KeyboardText const & text)
{
  std::ostringstream os;
  os << "Keyboard text [ " << StringUtf8Multilang::GetLangByCode(text.m_deviceLang) << ": "
     << text.m_text << " ]";
  os << "Keyboard locales [ ";
  for (auto const index : text.m_keyboardLangs)
    os << StringUtf8Multilang::GetLangByCode(index) << " ";

  os << " ]";
  return os.str();
}

struct Review
{
  using ReviewId = uint64_t;

  Review() = default;
  Review(ReviewId id, Text const & text, Author const & author, float const rating,
         Time const & time)
    : m_text(text), m_author(author), m_rating(rating), m_time(time)
  {
  }

  DECLARE_VISITOR(visitor(m_id, "id"), visitor(m_text, "text"), visitor(m_author, "author"),
                  visitor.VisitRating(m_rating, "rating"), visitor(m_time, "date"))

  bool operator==(Review const & rhs) const
  {
    return m_id == rhs.m_id && m_text == rhs.m_text && m_author == rhs.m_author &&
           m_rating == rhs.m_rating && m_time == rhs.m_time;
  }

  ReviewId m_id{};
  Text m_text{};
  Author m_author{};
  float m_rating{};
  Time m_time{};
};

inline std::string DebugPrint(Review const & review)
{
  std::ostringstream os;
  os << "Review [ ";
  os << "id:" << review.m_id << ", ";
  os << "text:" << DebugPrint(review.m_text) << ", ";
  os << "author:" << review.m_author << ", ";
  os << "rating:" << review.m_rating << ", ";
  os << "days since epoch:" << ToDaysSinceEpoch(review.m_time) << " ]";
  return os.str();
}

using Reviews = std::vector<Review>;

struct Attribute
{
  Attribute() = default;
  Attribute(TranslationKey const & key, TranslationKey const & value) : m_key(key), m_value(value)
  {
  }

  DECLARE_VISITOR_AND_DEBUG_PRINT(Attribute, visitor(m_key, "key"), visitor(m_value, "value"))

  bool operator==(Attribute const & rhs) const
  {
    return m_key == rhs.m_key && m_value == rhs.m_value;
  }

  TranslationKey m_key{};
  TranslationKey m_value{};
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

namespace v0
{
struct UGCUpdate
{
  UGCUpdate() = default;
  UGCUpdate(Ratings const & records, Text const & text, Time const & time)
    : m_ratings(records), m_text(text), m_time(time)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor(m_text, "text"), visitor(m_time, "date"))

  bool operator==(UGCUpdate const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_text == rhs.m_text && m_time == rhs.m_time;
  }

  bool IsEmpty() const
  {
    return (m_ratings.empty() && m_text.m_text.empty()) || m_time == Time();
  }

  bool IsValid() const
  {
    bool const timeIsValid = m_time != Time();
    if (!m_text.m_text.empty())
      return timeIsValid;

    bool ratingIsValid = false;
    for (auto const & r : m_ratings)
    {
      if (static_cast<int>(r.m_value) > 0)
      {
        ratingIsValid = true;
        break;
      }
    }

    return ratingIsValid && timeIsValid;
  }

  Ratings m_ratings;
  Text m_text;
  Time m_time{};
};

inline std::string DebugPrint(UGCUpdate const & ugcUpdate)
{
  std::ostringstream os;
  os << "UGCUpdate [ ";
  os << "records:" << ::DebugPrint(ugcUpdate.m_ratings) << ", ";
  os << "text:" << DebugPrint(ugcUpdate.m_text) << ", ";
  os << "days since epoch:" << ToDaysSinceEpoch(ugcUpdate.m_time) << " ]";
  return os.str();
}
}  // namespace v0

struct UGC
{
  UGC() = default;
  UGC(Ratings const & records, Reviews const & reviews, float const totalRating, uint32_t basedOn)
    : m_ratings(records), m_reviews(reviews), m_totalRating(totalRating), m_basedOn(basedOn)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor(m_reviews, "reviews"),
                  visitor.VisitRating(m_totalRating, "total_rating"),
                  visitor.VisitVarUint(m_basedOn, "based_on"))

  bool operator==(UGC const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_reviews == rhs.m_reviews &&
        my::AlmostEqualAbs(m_totalRating, rhs.m_totalRating, 1e-6f) && m_basedOn == rhs.m_basedOn;
  }

  bool IsEmpty() const
  {
    return m_reviews.empty() &&
           (m_ratings.empty() || m_totalRating <= kInvalidRatingValue || m_basedOn == 0);
  }

  Ratings m_ratings;
  Reviews m_reviews;
  float m_totalRating = kInvalidRatingValue;
  uint32_t m_basedOn{};
};

inline std::string DebugPrint(UGC const & ugc)
{
  std::ostringstream os;
  os << "UGC [ ";
  os << "total rating:" << ugc.m_totalRating << ", ";
  os << "based on:" << ugc.m_basedOn << ", ";
  os << "records:" << ::DebugPrint(ugc.m_ratings) << ", ";
  os << "reviews:" << ::DebugPrint(ugc.m_reviews) << " ]";
  return os.str();
}

struct UGCUpdate
{
  UGCUpdate() = default;
  UGCUpdate(Ratings const & records, KeyboardText const & text, Time const & time)
    : m_ratings(records), m_text(text), m_time(time)
  {
  }

  DECLARE_VISITOR(visitor(m_ratings, "ratings"), visitor(m_text, "text"), visitor(m_time, "date"))

  bool operator==(UGCUpdate const & rhs) const
  {
    return m_ratings == rhs.m_ratings && m_text == rhs.m_text && m_time == rhs.m_time;
  }

  void BuildFrom(v0::UGCUpdate const & update)
  {
    m_ratings = update.m_ratings;
    m_text.m_text = update.m_text.m_text;
    m_text.m_deviceLang = update.m_text.m_lang;
    m_time = update.m_time;
  }

  bool IsEmpty() const
  {
    return (m_ratings.empty() && m_text.m_text.empty()) || m_time == Time();
  }

  bool IsValid() const
  {
    bool const timeIsValid = m_time != Time();
    if (!m_text.m_text.empty())
      return timeIsValid;

    bool ratingIsValid = false;
    for (auto const & r : m_ratings)
    {
      if (static_cast<int>(r.m_value) > 0)
      {
        ratingIsValid = true;
        break;
      }
    }

    return ratingIsValid && timeIsValid;
  }

  Ratings m_ratings;
  KeyboardText m_text;
  Time m_time{};
};

inline std::string DebugPrint(UGCUpdate const & ugcUpdate)
{
  std::ostringstream os;
  os << "UGCUpdate [ ";
  os << "records:" << ::DebugPrint(ugcUpdate.m_ratings) << ", ";
  os << "text:" << DebugPrint(ugcUpdate.m_text) << ", ";
  os << "days since epoch:" << ToDaysSinceEpoch(ugcUpdate.m_time) << " ]";
  return os.str();
}

enum class IndexVersion : uint8_t
{
  V0 = 0,
  V1 = 1,
  Latest = V1
};

struct UpdateIndex
{
  DECLARE_VISITOR(visitor.VisitPoint(m_mercator, "x", "y"), visitor(m_type, "type"),
                  visitor(m_matchingType, "matching_type"),
                  visitor(m_offset, "offset"), visitor(m_deleted, "deleted"),
                  visitor(m_synchronized, "synchronized"), visitor(m_mwmName, "mwm_name"),
                  visitor(m_dataVersion, "data_version"), visitor(m_featureId, "feature_id"),
                  visitor(m_version, IndexVersion::V0, "version"))

  m2::PointD m_mercator = m2::PointD::Zero();
  // Index of the type from classificator.txt
  uint32_t m_type = 0;
  // Index of the type from ugc_types.txt
  uint32_t m_matchingType = 0;
  uint64_t m_offset = 0;
  bool m_deleted = false;
  bool m_synchronized = false;
  std::string m_mwmName;
  int64_t m_dataVersion = 0;
  uint32_t m_featureId = 0;
  IndexVersion m_version = IndexVersion::Latest;
};

using UpdateIndexes = std::vector<UpdateIndex>;
}  // namespace ugc

