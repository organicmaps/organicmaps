#include "generator/viator_dataset.hpp"

#include "generator/feature_builder.hpp"
#include "generator/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include "boost/algorithm/string/replace.hpp"

namespace
{
enum class TsvFields
{
  Id = 0,
  Name,
  Latitude,
  Longtitude,

  FieldsCount
};

static constexpr size_t FieldIndex(TsvFields field) { return static_cast<size_t>(field); }
static constexpr size_t FieldsCount() { return static_cast<size_t>(TsvFields::FieldsCount); }
}  // namespace

namespace generator
{
// ViatorCity -------------------------------------------------------------------------------------
ViatorCity::ViatorCity(std::string const & src)
{
  vector<std::string> rec;
  strings::ParseCSVRow(src, '\t', rec);
  CHECK_EQUAL(rec.size(), FieldsCount(),
              ("Error parsing viator cities, line:", boost::replace_all_copy(src, "\t", "\\t")));

  CLOG(LDEBUG, strings::to_uint(rec[FieldIndex(TsvFields::Id)], m_id.Get()), ());
  CLOG(LDEBUG, strings::to_double(rec[FieldIndex(TsvFields::Latitude)], m_latLon.lat), ());
  CLOG(LDEBUG, strings::to_double(rec[FieldIndex(TsvFields::Longtitude)], m_latLon.lon), ());

  m_name = rec[FieldIndex(TsvFields::Name)];
}

std::ostream & operator<<(std::ostream & s, ViatorCity const & h)
{
  s << std::fixed << std::setprecision(7);
  return s << "Id: " << h.m_id << "\t Name: " << h.m_name << "\t lat: " << h.m_latLon.lat
           << " lon: " << h.m_latLon.lon;
}

// ViatorDataset ----------------------------------------------------------------------------------
ViatorDataset::ViatorDataset(std::string const & dataPath)
  : m_storage(3000.0 /* distanceLimitMeters */, 3 /* maxSelectedElements */)
{
  LoadDataSource(m_dataSource);
  m_cityFinder = make_unique<search::CityFinder>(m_dataSource);

  m_storage.LoadData(dataPath);
}

ViatorCity::ObjectId ViatorDataset::FindMatchingObjectId(FeatureBuilder1 const & fb) const
{
  if (!ftypes::IsCityChecker::Instance()(fb.GetTypes()))
    return ViatorCity::InvalidObjectId();

  auto const name = m_cityFinder->GetCityName(fb.GetKeyPoint(), StringUtf8Multilang::kEnglishCode);

  auto const nearbyIds = m_storage.GetNearestObjects(MercatorBounds::ToLatLon(fb.GetKeyPoint()));

  for (auto const objId : nearbyIds)
  {
    auto const city = m_storage.GetObjectById(objId);
    if (name == city.m_name)
      return objId;

    auto const viatorName = m_cityFinder->GetCityName(MercatorBounds::FromLatLon(city.m_latLon),
                                                      StringUtf8Multilang::kEnglishCode);
    if (name == viatorName)
      return objId;
  }

  CLOG(LWARNING, nearbyIds.empty(),
       ("Viator city matching failed! OSM city:", name, "OSM point:", fb.GetKeyPoint(),
        "Viator cities:", nearbyIds));

  return ViatorCity::InvalidObjectId();
}

void ViatorDataset::PreprocessMatchedOsmObject(ViatorCity::ObjectId const matchedObjId,
                                               FeatureBuilder1 & fb,
                                               function<void(FeatureBuilder1 &)> const fn) const
{
  auto const & city = m_storage.GetObjectById(matchedObjId);
  auto & metadata = fb.GetMetadata();
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(city.m_id.Get()));

  auto const & clf = classif();
  FeatureParams & params = fb.GetParams();
  params.AddType(clf.GetTypeByPath({"sponsored", "viator"}));

  fn(fb);
}
}  // namespace generator
