#include "generator/utils.hpp"

#include "search/search_quality/helpers.hpp"

#include "search/categories_cache.hpp"
#include "search/engine.hpp"
#include "search/locality_finder.hpp"
#include "search/reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

using namespace std;

class ClosestPoint
{
  m2::PointD const & m_center;
  m2::PointD m_best;
  double m_distance = numeric_limits<double>::max();

public:
  explicit ClosestPoint(m2::PointD const & center) : m_center(center), m_best(0, 0) {}

  m2::PointD GetBest() const { return m_best; }

  void operator()(m2::PointD const & point)
  {
    double distance = m_center.SquaredLength(point);
    if (distance < m_distance)
    {
      m_distance = distance;
      m_best = point;
    }
  }
};

m2::PointD FindCenter(FeatureType & f)
{
  ClosestPoint closest(f.GetLimitRect(FeatureType::BEST_GEOMETRY).Center());
  if (f.GetGeomType() == feature::GeomType::Area)
  {
    f.ForEachTriangle([&closest](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    { closest((p1 + p2 + p3) / 3); }, FeatureType::BEST_GEOMETRY);
  }
  else
  {
    f.ForEachPoint(closest, FeatureType::BEST_GEOMETRY);
  }
  return closest.GetBest();
}

size_t const kLangCount = StringUtf8Multilang::GetSupportedLanguages().size();

std::string GetReadableType(FeatureType & f)
{
  std::string result;
  f.ForEachType([&](uint32_t const type)
  {
    if (ftypes::IsPoiChecker::Instance()(type) || ftypes::IsPlaceChecker::Instance()(type))
      result = classif().GetReadableObjectName(type);
  });
  return result;
}

void AppendNames(FeatureType & f, vector<std::string> & columns)
{
  vector<std::string> names(kLangCount);
  f.GetNames().ForEach([&names](int8_t const code, string_view const name) { names[code] = name; });
  columns.insert(columns.end(), next(names.begin()), names.end());
}

void PrintAsCSV(vector<std::string> const & columns, char const delimiter, ostream & out)
{
  bool first = true;
  for (std::string value : columns)
  {
    // Newlines are hard to process, replace them with spaces. And trim the
    // string.
    ranges::replace(value, '\r', ' ');
    ranges::replace(value, '\n', ' ');
    strings::Trim(value);

    if (first)
      first = false;
    else
      out << delimiter;
    bool needsQuotes = value.find('"') != std::string::npos || value.find(delimiter) != std::string::npos;
    if (!needsQuotes)
    {
      out << value;
    }
    else
    {
      size_t pos = 0;
      while ((pos = value.find('"', pos)) != std::string::npos)
      {
        value.insert(pos, 1, '"');
        pos += 2;
      }
      out << '"' << value << '"';
    }
  }
  out << endl;
}

class Processor
{
  search::ReverseGeocoder m_geocoder;
  base::Cancellable m_cancellable;
  search::CitiesBoundariesTable m_boundariesTable;
  search::VillagesCache m_villagesCache;
  search::LocalityFinder m_finder;

public:
  explicit Processor(DataSource const & dataSource)
    : m_geocoder(dataSource)
    , m_boundariesTable(dataSource)
    , m_villagesCache(m_cancellable)
    , m_finder(dataSource, m_boundariesTable, m_villagesCache)
  {
    m_boundariesTable.Load();
  }

  void ClearCache() { m_villagesCache.Clear(); }

  void operator()(FeatureType & f) { Process(f); }

  void Process(FeatureType & f)
  {
    f.ParseAllBeforeGeometry();

    std::string const & category = GetReadableType(f);
    auto const & meta = f.GetMetadata();

    auto const metaOperator = meta.Get(feature::Metadata::FMD_OPERATOR);
    if ((!f.HasName() && metaOperator.empty()) ||
        (f.GetGeomType() == feature::GeomType::Line && category != "highway-pedestrian") || category.empty())
    {
      return;
    }
    m2::PointD const & center = FindCenter(f);
    ms::LatLon const & ll = mercator::ToLatLon(center);
    osm::MapObject obj;
    obj.SetFromFeatureType(f);

    string_view city;
    if (auto loc = m_finder.GetBestLocality(center))
      loc->GetSpecifiedOrDefaultName(StringUtf8Multilang::kDefaultCode, city);

    std::string name{f.GetDefaultName()};
    if (name.empty())
    {
      name = f.GetReadableName();
      if (name.empty())
        name = metaOperator;
    }

    std::string const & lat = strings::to_string_dac(ll.m_lat, 6);
    std::string const & lon = strings::to_string_dac(ll.m_lon, 6);
    search::ReverseGeocoder::Address addr;
    std::string addrStreet;
    std::string addrHouse;
    double constexpr kDistanceThresholdMeters = 0.5;
    if (m_geocoder.GetExactAddress(f, addr))
    {
      addrStreet = addr.GetStreetName();
      addrHouse = addr.GetHouseNumber();
    }
    else
    {
      m_geocoder.GetNearbyAddress(center, addr);
      if (addr.GetDistance() < kDistanceThresholdMeters)
      {
        addrStreet = addr.GetStreetName();
        addrHouse = addr.GetHouseNumber();
      }
    }
    std::string const phone(meta.Get(feature::Metadata::FMD_PHONE_NUMBER));
    std::string const website(meta.Get(feature::Metadata::FMD_WEBSITE));
    std::string const contact_facebook(meta.Get(feature::Metadata::FMD_CONTACT_FACEBOOK));
    std::string const contact_instagram(meta.Get(feature::Metadata::FMD_CONTACT_INSTAGRAM));
    std::string const contact_twitter(meta.Get(feature::Metadata::FMD_CONTACT_TWITTER));
    std::string const contact_vk(meta.Get(feature::Metadata::FMD_CONTACT_VK));
    std::string const contact_line(meta.Get(feature::Metadata::FMD_CONTACT_LINE));
    std::string const stars(meta.Get(feature::Metadata::FMD_STARS));
    std::string const internet(meta.Get(feature::Metadata::FMD_INTERNET));
    std::string const denomination(meta.Get(feature::Metadata::FMD_DENOMINATION));
    std::string const wheelchair(
        DebugPrint(feature::GetWheelchairType(feature::TypesHolder(f)).value_or(ftraits::WheelchairAvailability::No)));
    std::string const opening_hours(meta.Get(feature::Metadata::FMD_OPEN_HOURS));
    std::string const wikipedia(meta.Get(feature::Metadata::FMD_WIKIPEDIA));
    std::string const wikimedia_commons(meta.Get(feature::Metadata::FMD_WIKIMEDIA_COMMONS));
    std::string const floor(meta.Get(feature::Metadata::FMD_LEVEL));
    std::string const fee = category.ends_with("-fee") ? "yes" : "";
    std::string const atm = feature::HasAtm(feature::TypesHolder(f)) ? "yes" : "";

    vector columns = {lat,
                      lon,
                      category,
                      name,
                      std::string(city),
                      addrStreet,
                      addrHouse,
                      phone,
                      website,
                      stars,
                      std::string(metaOperator),
                      internet,
                      denomination,
                      wheelchair,
                      opening_hours,
                      wikipedia,
                      floor,
                      fee,
                      atm,
                      contact_facebook,
                      contact_instagram,
                      contact_twitter,
                      contact_vk,
                      contact_line,
                      wikimedia_commons};

    AppendNames(f, columns);
    PrintAsCSV(columns, ';', cout);
  }
};

void PrintHeader()
{
  vector<std::string> columns = {"lat",
                                 "lon",
                                 "mwm",
                                 "category",
                                 "name",
                                 "city",
                                 "street",
                                 "house",
                                 "phone",
                                 "website",
                                 "cuisines",
                                 "stars",
                                 "operator",
                                 "internet",
                                 "denomination",
                                 "wheelchair",
                                 "opening_hours",
                                 "wikipedia",
                                 "floor",
                                 "fee",
                                 "atm",
                                 "contact_facebook",
                                 "contact_instagram",
                                 "contact_twitter",
                                 "contact_vk",
                                 "contact_line",
                                 "wikimedia_commons"};
  // Append all supported name languages in order.
  for (size_t idx = 1; idx < kLangCount; idx++)
    columns.push_back("name_" + std::string(StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(idx))));
  PrintAsCSV(columns, ';', cout);
}

void PrintMwmMetadata(feature::RegionData const & data)
{
  std::vector<int8_t> langs;
  data.GetLanguages(langs);
  std::string langsStr;
  for (auto const & l : langs)
  {
    langsStr += StringUtf8Multilang::GetLangByCode(l);
    langsStr += ",";
  }
  LOG(LINFO, ("Mwm metadata:"));
  LOG(LINFO, ("  RD_DRIVING:", data.Get(feature::RegionData::Type::RD_DRIVING)));
  LOG(LINFO, ("  RD_LANGUAGES:", langsStr));
  LOG(LINFO, ("  RD_TIMEZONE:", data.Get(feature::RegionData::Type::RD_TIMEZONE)));
  LOG(LINFO, ("  RD_ADDRESS_FORMAT:", data.Get(feature::RegionData::Type::RD_ADDRESS_FORMAT)));
  LOG(LINFO, ("  RD_PHONE_FORMAT:", data.Get(feature::RegionData::Type::RD_PHONE_FORMAT)));
  LOG(LINFO, ("  RD_PUBLIC_HOLIDAYS:", data.Get(feature::RegionData::Type::RD_PUBLIC_HOLIDAYS)));
  LOG(LINFO, ("  RD_ALLOW_HOUSENAMES:", data.Get(feature::RegionData::Type::RD_ALLOW_HOUSENAMES)));
  LOG(LINFO, ("  RD_LEAP_WEIGHT_SPEED:", data.Get(feature::RegionData::Type::RD_LEAP_WEIGHT_SPEED)));
}

void PrintMwmInfo(std::shared_ptr<MwmInfo> const & info)
{
  auto const version = MwmValue{info->GetLocalFile()}.GetMwmVersion();
  LOG(LINFO, ("MwmInfo:", info->GetCountryName()));
  LOG(LINFO, ("  format:", version.GetFormat()));
  LOG(LINFO, ("  version:", version.GetVersion()));
  PrintMwmMetadata(info->GetRegionData());
}

int main(int const argc, char const ** const argv)
{
  if (argc != 2)
  {
    LOG(LERROR, ("Usage: mwm_viewer <mwm_path>"));
    return 1;
  }

  std::filesystem::path const mwmPath = argv[1];

  GetStyleReader().SetCurrentStyle(MapStyleMerged);
  classificator::Load();

  FrozenDataSource dataSource;
  dataSource.RegisterMap(platform::LocalCountryFile{mwmPath.parent_path(), platform::CountryFile(mwmPath.stem()), 0});
  std::vector<std::shared_ptr<MwmInfo>> mwmInfos;
  dataSource.GetMwmsInfo(mwmInfos);
  Processor doProcess(dataSource);
  for (auto const & mwmInfo : mwmInfos)
  {
    LOG(LINFO, ("Processing", mwmInfo->GetCountryName()));
    MwmSet::MwmId mwmId(mwmInfo);
    PrintMwmInfo(mwmInfo);
    PrintHeader();
    FeaturesLoaderGuard loader(dataSource, mwmId);
    for (uint32_t ftIndex = 0; ftIndex < loader.GetNumFeatures(); ftIndex++)
      if (auto ft = loader.GetFeatureByIndex(ftIndex))
        doProcess.Process(*ft);
  }
}
