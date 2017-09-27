#include "ugc_translator.hpp"

#include "ugc_db.hpp"

#include "3party/jansson/myjansson.hpp"

namespace generator
{
UGCTranslator::UGCTranslator() : m_dbRatings(":memory:"), m_dbReviews(":memory:") {}

UGCTranslator::UGCTranslator(std::string const & path)
  : m_dbRatings(path + ".ratings"), m_dbReviews(path + ".reviews")
{
}

bool UGCTranslator::TranslateUGC(osm::Id const & id, ugc::UGC & ugc)
{
  bool ratingsOk = TranslateRatings(m_dbRatings, id, ugc.m_ratings);
  bool reviewsOk = TranslateReview(m_dbReviews, id, ugc.m_reviews);
  return ratingsOk && reviewsOk;
}

void UGCTranslator::CreateRatings(std::string const & data)
{
  bool rc = m_dbRatings.Exec(data);
  UNUSED_VALUE(rc);
}

void UGCTranslator::CreateReviews(std::string const & data)
{
  bool rc = m_dbReviews.Exec(data);
  UNUSED_VALUE(rc);
}

bool UGCTranslator::TranslateRatings(UGCDB & db, osm::Id const id, ugc::Ratings & ratings)
{
  std::vector<uint8_t> blob;
  bool rc = db.Get(id, blob);
  if (!rc)
    return false;
  std::string result(blob.cbegin(), blob.cend());

  my::Json jsonRoot(result);

  size_t size = json_array_size(jsonRoot.get());
  for (size_t i = 0; i < size; ++i)
  {
    json_t * el = json_array_get(jsonRoot.get(), i);
    double ratingValue = 0;
    size_t translationKeyId = 0;

    FromJSONObject(el, "value", ratingValue);
    FromJSONObject(el, "criterion_id", translationKeyId);

    std::ostringstream translationKey;
    translationKey << "TranslationKey" << translationKeyId;
    ratings.emplace_back(translationKey.str(), static_cast<float>(ratingValue));
  }

  return true;
}

bool UGCTranslator::TranslateReview(UGCDB & db, osm::Id const id, std::vector<ugc::Review> & review)
{
  return true;
}

// bool UGCTranslator::TranslateAttribute(UGCDB & db, osm::Id const id, ugc::Attribute & attribute)
//{
//  return false;
//}
}  // namespace generator
