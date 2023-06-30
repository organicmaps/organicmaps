#include "generator/kayak_dataset.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"


namespace generator
{
using namespace feature;

// BookingHotel ------------------------------------------------------------------------------------
KayakHotel::KayakHotel(std::string src)
{
  using namespace strings;

  // Patch strange entries.
  if (StartsWith(src, "\","))
    src.erase(0, 1);

  /// @todo For fast parsing we can preprocess src (quotes) and return string_view's.
  std::vector<std::string> rec;
  strings::ParseCSVRow(src, ',', rec);

  // Skip bad entries and header.
  if (rec.size() != Fields::Counter || rec[0] == "ChainID")
    return;

  // Assign id in the end in case of possible errors.
  uint32_t id;
  CLOG(LDEBUG, to_uint(rec[Fields::KayakHotelID], id), ()); /// @todo HotelID ?
  CLOG(LDEBUG, to_double(rec[Fields::Latitude], m_latLon.m_lat), (rec[Fields::Latitude]));
  CLOG(LDEBUG, to_double(rec[Fields::Longitude], m_latLon.m_lon), (rec[Fields::Longitude]));

  if (!to_double(rec[Fields::OverallRating], m_overallRating))
    m_overallRating = kInvalidRating;

  m_name = rec[Fields::HotelName];
  m_address = rec[Fields::HotelAddress];

  m_id.Set(id);
}


// KayakDataset ----------------------------------------------------------------------------------
template <>
bool KayakDataset::IsSponsoredCandidate(FeatureBuilder const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsHotelChecker::Instance()(fb.GetTypes());
}

template <>
void KayakDataset::PreprocessMatchedOsmObject(ObjectId id, FeatureBuilder & fb, FBuilderFnT const fn) const
{
  auto const & hotel = m_storage.GetObjectById(id);

  fb.SetHotelInfo(Metadata::SRC_KAYAK, hotel.m_id.Get(), hotel.m_overallRating, 0 /* priceCategory */);

  fn(fb);
}

template <>
void KayakDataset::BuildObject(Object const &, FBuilderFnT const &) const
{
  // Don't create new objects.
}

} // namespace generator
