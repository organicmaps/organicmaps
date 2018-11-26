#include "generator/opentable_dataset.hpp"

#include "generator/feature_builder.hpp"
#include "generator/sponsored_scoring.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <iomanip>
#include <iostream>

#include "boost/algorithm/string/replace.hpp"

namespace generator
{
// OpentableRestaurant ------------------------------------------------------------------------------
OpentableRestaurant::OpentableRestaurant(std::string const & src)
{
  vector<std::string> rec;
  strings::ParseCSVRow(src, '\t', rec);
  CHECK_EQUAL(rec.size(), FieldsCount(), ("Error parsing restaurants.tsv line:",
                                          boost::replace_all_copy(src, "\t", "\\t")));

  CLOG(LDEBUG, strings::to_uint(rec[FieldIndex(Fields::Id)], m_id.Get()), ());
  CLOG(LDEBUG, strings::to_double(rec[FieldIndex(Fields::Latitude)], m_latLon.lat), ());
  CLOG(LDEBUG, strings::to_double(rec[FieldIndex(Fields::Longtitude)], m_latLon.lon), ());

  m_name = rec[FieldIndex(Fields::Name)];
  m_address = rec[FieldIndex(Fields::Address)];
  m_descUrl = rec[FieldIndex(Fields::DescUrl)];
}

// OpentableDataset ---------------------------------------------------------------------------------
template <>
bool OpentableDataset::NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsEatChecker::Instance()(fb.GetTypes());
}

template <>
void OpentableDataset::PreprocessMatchedOsmObject(ObjectId const matchedObjId, FeatureBuilder1 & fb,
                                                  function<void(FeatureBuilder1 &)> const fn) const
{
  auto const & restaurant = m_storage.GetObjectById(matchedObjId);
  auto & metadata = fb.GetMetadata();
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(restaurant.m_id.Get()));

  FeatureParams & params = fb.GetParams();
  // params.AddAddress(restaurant.address);
  // TODO(mgsergio): addr:full ???

  params.AddName(StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode),
                 restaurant.m_name);

  auto const & clf = classif();
  params.AddType(clf.GetTypeByPath({"sponsored", "opentable"}));

  fn(fb);
}

template <>
OpentableDataset::ObjectId OpentableDataset::FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const
{
  auto const name = fb.GetName(StringUtf8Multilang::kDefaultCode);

  if (name.empty())
    return Object::InvalidObjectId();

  // Find |kMaxSelectedElements| nearest values to a point.
  auto const nearbyIds = m_storage.GetNearestObjects(MercatorBounds::ToLatLon(fb.GetKeyPoint()));

  for (auto const objId : nearbyIds)
  {
    if (sponsored_scoring::Match(m_storage.GetObjectById(objId), fb).IsMatched())
      return objId;
  }

  return Object::InvalidObjectId();
}
}  // namespace generator
