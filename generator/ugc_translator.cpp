#include "generator/ugc_translator.hpp"

#include "generator/ugc_db.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
void FillRatings(json_t const * ratings, ugc::Ratings & result)
{
  size_t size = json_array_size(ratings);
  for (size_t i = 0; i < size; ++i)
  {
    json_t * el = json_array_get(ratings, i);
    uint32_t id = 0;
    double ratingValue = 0.;

    FromJSONObject(el, "id", id);
    FromJSONObject(el, "value", ratingValue);

    result.emplace_back(strings::to_string(id), static_cast<float>(ratingValue));
  }
}

void FillReviews(json_t const * reviews, ugc::Reviews & result)
{
  size_t size = json_array_size(reviews);
  for (size_t i = 0; i < size; ++i)
  {
    ugc::Review review;

    json_t * el = json_array_get(reviews, i);

    uint32_t daysSinceEpoch = 0;
    double rating = 0.;
    std::string lang;

    FromJSONObject(el, "id", review.m_id);
    FromJSONObject(el, "text", review.m_text.m_text);
    FromJSONObject(el, "lang", lang);
    FromJSONObject(el, "author", review.m_author);
    FromJSONObject(el, "rating", rating);
    FromJSONObject(el, "date", daysSinceEpoch);

    review.m_text.m_lang = StringUtf8Multilang::GetLangIndex(lang);
    review.m_rating = rating;
    review.m_time = ugc::Clock::now() - std::chrono::hours(daysSinceEpoch * 24);

    result.push_back(std::move(review));
  }
}
}  // namespace

namespace generator
{
UGCTranslator::UGCTranslator() : m_db(":memory:") {}

UGCTranslator::UGCTranslator(std::string const & dbFilename) : m_db(dbFilename) {}

bool UGCTranslator::TranslateUGC(osm::Id const & id, ugc::UGC & ugc)
{
  std::vector<uint8_t> blob;
  bool rc = m_db.Get(id, blob);
  if (!rc)
    return false;
  std::string result(blob.cbegin(), blob.cend());

  try
  {
    my::Json root(result);
    double totalRating = 0.;

    FromJSONObject(root.get(), "total_rating", totalRating);
    FromJSONObject(root.get(), "votes", ugc.m_votes);

    ugc.m_totalRating = totalRating;

    auto const ratings = json_object_get(root.get(), "ratings");
    auto const reviews = json_object_get(root.get(), "reviews");

    if (!CheckJsonArray(ratings) || !CheckJsonArray(reviews))
      return false;

    FillRatings(ratings, ugc.m_ratings);
    FillReviews(reviews, ugc.m_reviews);
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    ugc = {};
    return false;
  }

  return true;
}

void UGCTranslator::CreateDb(std::string const & data)
{
  bool rc = m_db.Exec(data);
  UNUSED_VALUE(rc);
}
}  // namespace generator
