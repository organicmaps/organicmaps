#include "generator/kayak_dataset.hpp"

#include "generator/feature_builder.hpp"
#include "generator/sponsored_dataset_inl.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"


namespace generator
{
using namespace feature;

// KayakHotel ------------------------------------------------------------------------------------
KayakHotel::KayakHotel(std::string src)
{
  using namespace strings;

  // Patch strange entries.
  if (StartsWith(src, "\","))
    src.erase(0, 1);

  /// @todo For fast parsing we can preprocess src (quotes) and return string_view's.
  std::vector<std::string> rec;
  ParseCSVRow(src, ',', rec);

  // Skip bad entries and header.
  if (rec.size() != Fields::Counter || rec[0] == "ChainID")
    return;

  // Assign id in the end in case of possible errors.
  uint32_t id;
  CLOG(LDEBUG, to_uint(rec[Fields::KayakHotelID], id), ());
  CLOG(LDEBUG, to_uint(rec[Fields::PlaceID], m_placeID), ());
  CLOG(LDEBUG, to_double(rec[Fields::Latitude], m_latLon.m_lat), (rec[Fields::Latitude]));
  CLOG(LDEBUG, to_double(rec[Fields::Longitude], m_latLon.m_lon), (rec[Fields::Longitude]));

  if (!to_double(rec[Fields::OverallRating], m_overallRating))
    m_overallRating = kInvalidRating;

  m_name = rec[Fields::HotelName];
  m_address = rec[Fields::HotelAddress];

  m_id.Set(id);
}

// KayakPlace ----------------------------------------------------------------------------------
KayakPlace::KayakPlace(std::string src)
{
  using namespace strings;

  std::vector<std::string> rec;
  ParseCSVRow(src, ',', rec);

  if (rec.size() != Fields::Counter || rec[0] == "CountryCode")
    return;

  m_good = to_uint(rec[Fields::PlaceID], m_placeID) &&
           to_uint(rec[Fields::KayakPlaceID], m_kayakPlaceID);
}

std::string DebugPrint(KayakPlace const & p)
{
  return std::to_string(p.m_placeID) + "; " + std::to_string(p.m_kayakPlaceID);
}

// KayakDataset ----------------------------------------------------------------------------------
KayakDataset::KayakDataset(std::string const & hotelsPath, std::string const & placesPath)
  : BaseDatasetT(hotelsPath)
{
  std::ifstream source(placesPath);
  if (!source)
  {
    LOG(LERROR, ("Error while opening", placesPath, ":", strerror(errno)));
    return;
  }

  for (std::string line; std::getline(source, line);)
  {
    KayakPlace place(std::move(line));
    line.clear();

    if (place.m_good)
      CLOG(LDEBUG, m_place2kayak.emplace(place.m_placeID, place.m_kayakPlaceID).second, (place));
  }
}

template <>
bool BaseDatasetT::IsSponsoredCandidate(FeatureBuilder const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsHotelChecker::Instance()(fb.GetTypes());
}

template <>
void BaseDatasetT::PreprocessMatchedOsmObject(ObjectId id, FeatureBuilder & fb, FBuilderFnT const fn) const
{
  auto const & hotel = m_storage.GetObjectById(id);

  // Only hack like this ..
  KayakDataset const & kds = static_cast<KayakDataset const &>(*this);
  uint32_t const cityID = kds.GetKayakPlaceID(hotel.m_placeID);
  if (cityID)
  {
    std::string uri = hotel.m_name + ",-c" + std::to_string(cityID) + "-h" + std::to_string(hotel.m_id.Get());
    fb.SetHotelInfo(std::move(uri), hotel.m_overallRating);
  }
  else
    LOG(LWARNING, ("Unknown PlaceID", hotel.m_placeID));

  fn(fb);
}

template <>
void BaseDatasetT::BuildObject(Object const &, FBuilderFnT const &) const
{
  // Don't create new objects.
}

} // namespace generator
