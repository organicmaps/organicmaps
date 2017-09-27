#pragma once

#include "ugc_db.hpp"

#include "ugc/types.hpp"
#include "generator/osm_id.hpp"

namespace generator
{
class UGCTranslator
{
public:
  UGCTranslator();
  UGCTranslator(std::string const & path);

  bool TranslateUGC(osm::Id const & id, ugc::UGC & ugc);
  // For testing only
  void CreateRatings(std::string const & data);
  void CreateReviews(std::string const & data);
  
private:
  bool TranslateRatings(UGCDB & db, osm::Id const id, ugc::Ratings & ratings);
  bool TranslateReview(UGCDB & db, osm::Id const id, std::vector<ugc::Review> & review);
//  bool TranslateAttribute(UGCDB & db, osm::Id const id, ugc::Attribute & attribute);

  UGCDB m_dbRatings;
  UGCDB m_dbReviews;
};
}  // namespace generator
