#include "generator/opentable_dataset.hpp"

//#include "generator/openatble_scoring.hpp" // or just sonsored scoring
#include "generator/feature_builder.hpp"

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

  // TODO(mgsergio): Handle all types of restaurants:
  // bar cafe (fast_food ??) pub restaurant
  // return ftypes::IsRestaurantChecker::Instance()(fb.GetTypes());
  return true;
}

// TODO(mgsergio): Try to eliminate as much code duplication as possible. (See booking_dataset.cpp)
template <>
void OpentableDataset::BuildObject(Object const & restaurant,
                                 function<void(FeatureBuilder1 &)> const & fn) const
{
  FeatureBuilder1 fb;
  FeatureParams params;

  fb.SetCenter(MercatorBounds::FromLatLon(restaurant.m_lat, restaurant.m_lon));

  auto & metadata = params.GetMetadata();
  // TODO(mgsergio): Rename FMD_SPONSORED_ID to FMD_BOOKING_ID.
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(restaurant.m_id.Get()));
  metadata.Set(feature::Metadata::FMD_WEBSITE, restaurant.m_descUrl);

  // params.AddAddress(restaurant.address);
  // TODO(mgsergio): addr:full ???

  if (!restaurant.m_street.empty())
    fb.AddStreet(restaurant.m_street);

  if (!restaurant.m_houseNumber.empty())
    fb.AddHouseNumber(restaurant.m_houseNumber);

  params.AddName(StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode),
                 restaurant.m_name);

  auto const & clf = classif();
  params.AddType(clf.GetTypeByPath({"sponsored", "booking"}));

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
  auto const bookingIndexes = GetNearestObjects(MercatorBounds::ToLatLon(fb.GetKeyPoint()),
                                                kMaxSelectedElements, kDistanceLimitInMeters);

  CHECK(false, ("Not implemented yet"));
  // for (auto const j : bookingIndexes)
  // {
  //   if (booking_scoring::Match(GetObjectById(j), fb).IsMatched())
  //     return j;
  // }

  return Object::InvalidObjectId();
}
}  // namespace generator
