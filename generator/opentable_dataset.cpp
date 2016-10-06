#include "generator/opentable_dataset.hpp"

#include "generator/feature_builder.hpp"
#include "generator/sponsored_scoring.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/string_utils.hpp"

namespace generator
{
namespace
{
string EscapeTabs(string const & str)
{
  stringstream ss;
  for (char c : str)
  {
    if (c == '\t')
      ss << "\\t";
    else
      ss << c;
  }
  return ss.str();
}
}  // namespace

// OpentableRestaurant ------------------------------------------------------------------------------

OpentableRestaurant::OpentableRestaurant(string const & src)
{
  vector<string> rec;
  strings::ParseCSVRow(src, '\t', rec);
  CHECK_EQUAL(rec.size(), FieldsCount(), ("Error parsing restaurants.tsv line:", EscapeTabs(src)));

  strings::to_uint(rec[Index(Fields::Id)], m_id.Get());
  // TODO(mgsergio): Use ms::LatLon.
  strings::to_double(rec[Index(Fields::Latitude)], m_lat);
  strings::to_double(rec[Index(Fields::Longtitude)], m_lon);

  m_name = rec[Index(Fields::Name)];
  m_address = rec[Index(Fields::Address)];

  m_descUrl = rec[Index(Fields::DescUrl)];
}

ostream & operator<<(ostream & s, OpentableRestaurant const & h)
{
  s << fixed << setprecision(7);
  return s << "Id: " << h.m_id << "\t Name: " << h.m_name << "\t Address: " << h.m_address
           << "\t lat: " << h.m_lat << " lon: " << h.m_lon;
}

// OpentableDataset ---------------------------------------------------------------------------------

template <>
bool OpentableDataset::NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsFoodChecker::Instance()(fb.GetTypes());
}

template <>
void OpentableDataset::PreprocessMatchedOsmObject(ObjectId const matchedObjId, FeatureBuilder1 & fb,
                                                  function<void(FeatureBuilder1 &)> const fn) const
{
  FeatureParams params = fb.GetParams();

  auto restaurant = GetObjectById(matchedObjId);
  auto & metadata = params.GetMetadata();
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(restaurant.m_id.Get()));
  metadata.Set(feature::Metadata::FMD_WEBSITE, restaurant.m_descUrl);

  // params.AddAddress(restaurant.address);
  // TODO(mgsergio): addr:full ???

  params.AddName(StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode),
                 restaurant.m_name);

  auto const & clf = classif();
  params.AddType(clf.GetTypeByPath({"sponsored", "opentable"}));

  fb.SetParams(params);

  fn(fb);
}

template <>
OpentableDataset::ObjectId OpentableDataset::FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const
{
  auto const name = fb.GetName(StringUtf8Multilang::kDefaultCode);

  if (name.empty())
    return Object::InvalidObjectId();

  // Find |kMaxSelectedElements| nearest values to a point.
  auto const nearbyIds = GetNearestObjects(MercatorBounds::ToLatLon(fb.GetKeyPoint()),
                                           kMaxSelectedElements, kDistanceLimitInMeters);

  for (auto const objId : nearbyIds)
  {
    if (sponsored_scoring::Match(GetObjectById(objId), fb).IsMatched())
      return objId;
  }

  return Object::InvalidObjectId();
}
}  // namespace generator
