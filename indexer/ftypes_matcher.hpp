#pragma once

#include "indexer/feature_data.hpp"

#include "base/base.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace feature { class TypesHolder; }
class FeatureType;

#define DECLARE_CHECKER_INSTANCE(CheckerType) static CheckerType const & Instance() { \
                                              static CheckerType const inst; return inst; }

namespace ftypes
{
class BaseChecker
{
protected:
  uint8_t const m_level;
  std::vector<uint32_t> m_types;

  BaseChecker(uint8_t level = 2) : m_level(level) {}
  virtual ~BaseChecker() = default;

public:
  virtual bool IsMatched(uint32_t type) const;
  virtual void ForEachType(std::function<void(uint32_t)> && fn) const;

  std::vector<uint32_t> const & GetTypes() const { return m_types; }

  bool operator()(feature::TypesHolder const & types) const;
  bool operator()(FeatureType & ft) const;
  bool operator()(std::vector<uint32_t> const & types) const;
  bool operator()(uint32_t type) const { return IsMatched(type); }

  static uint32_t PrepareToMatch(uint32_t type, uint8_t level);
};

class IsPeakChecker : public BaseChecker
{
  IsPeakChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsPeakChecker);
};

class IsATMChecker : public BaseChecker
{
  IsATMChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsATMChecker);
};

class IsPaymentTerminalChecker : public BaseChecker
{
  IsPaymentTerminalChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPaymentTerminalChecker);
};

class IsMoneyExchangeChecker : public BaseChecker
{
  IsMoneyExchangeChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsMoneyExchangeChecker);
};

class IsSpeedCamChecker : public BaseChecker
{
  IsSpeedCamChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsSpeedCamChecker);
};

class IsPostBoxChecker : public BaseChecker
{
  IsPostBoxChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPostBoxChecker);
};

class IsPostOfficeChecker : public BaseChecker
{
  IsPostOfficeChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPostOfficeChecker);
};

class IsFuelStationChecker : public BaseChecker
{
  IsFuelStationChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsFuelStationChecker);
};

class IsCarSharingChecker : public BaseChecker
{
  IsCarSharingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCarSharingChecker);
};

class IsCarRentalChecker : public BaseChecker
{
  IsCarRentalChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCarRentalChecker);
};

class IsBicycleRentalChecker : public BaseChecker
{
  IsBicycleRentalChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBicycleRentalChecker);
};

class IsRecyclingCentreChecker : public BaseChecker
{
  IsRecyclingCentreChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingCentreChecker);

  uint32_t GetType() const;
};

class IsRecyclingContainerChecker : public BaseChecker
{
  IsRecyclingContainerChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingContainerChecker);

  uint32_t GetType() const;
};

class IsRailwayStationChecker : public BaseChecker
{
  IsRailwayStationChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsRailwayStationChecker);
};

class IsSubwayStationChecker : public BaseChecker
{
  IsSubwayStationChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSubwayStationChecker);
};

class IsAirportChecker : public BaseChecker
{
  IsAirportChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAirportChecker);
};

class IsSquareChecker : public BaseChecker
{
  IsSquareChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSquareChecker);
};

// Type of suburb.
// Do not change values and order - they are in the order of decreasing specificity.
// Suburb > Neighbourhood > Residential
enum class SuburbType
{
  None = -1,
  Residential = 0,
  Neighbourhood,
  Suburb,
  Count
};

static_assert(base::Underlying(SuburbType::Residential) <
                  base::Underlying(SuburbType::Neighbourhood),
              "");
static_assert(base::Underlying(SuburbType::Neighbourhood) < base::Underlying(SuburbType::Suburb),
              "");

class IsSuburbChecker : public BaseChecker
{
  IsSuburbChecker();

public:
  SuburbType GetType(uint32_t t) const;
  SuburbType GetType(feature::TypesHolder const & types) const;
  SuburbType GetType(FeatureType & f) const;

  DECLARE_CHECKER_INSTANCE(IsSuburbChecker);
};

class IsWayChecker : public BaseChecker
{
  IsWayChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsWayChecker);
};

class IsStreetOrSquareChecker : public BaseChecker
{
  IsStreetOrSquareChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsStreetOrSquareChecker);
};

class IsAddressObjectChecker : public BaseChecker
{
  IsAddressObjectChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsAddressObjectChecker);
};

class IsVillageChecker : public BaseChecker
{
  IsVillageChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsVillageChecker);
};

class IsOneWayChecker : public BaseChecker
{
  IsOneWayChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsOneWayChecker);
};

class IsRoundAboutChecker : public BaseChecker
{
  IsRoundAboutChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsRoundAboutChecker);
};

class IsLinkChecker : public BaseChecker
{
  IsLinkChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsLinkChecker);
};

class IsBuildingChecker : public BaseChecker
{
  IsBuildingChecker();
public:
  uint32_t GetMainType() const { return m_types[0]; }
  DECLARE_CHECKER_INSTANCE(IsBuildingChecker);
};

class IsBuildingPartChecker : public ftypes::BaseChecker
{
  IsBuildingPartChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsBuildingPartChecker);
};

class IsIsolineChecker : public BaseChecker
{
  IsIsolineChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsIsolineChecker);
};

class IsPoiChecker : public BaseChecker
{
  IsPoiChecker();
public:
  static std::set<std::string> const kPoiTypes;

  DECLARE_CHECKER_INSTANCE(IsPoiChecker);
};

class AttractionsChecker : public BaseChecker
{
  AttractionsChecker();

public:
  std::vector<uint32_t> m_primaryTypes;
  std::vector<uint32_t> m_additionalTypes;

  DECLARE_CHECKER_INSTANCE(AttractionsChecker);

  template <typename Ft>
  bool NeedFeature(Ft & feature) const
  {
    bool need = false;
    feature.ForEachType([&](uint32_t type) {
      if (!need && IsMatched(type))
        need = true;
    });
    return need;
  }

  uint32_t GetBestType(FeatureParams::Types const & types) const;
};

class IsPlaceChecker : public BaseChecker
{
  IsPlaceChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsPlaceChecker);
};

class IsBridgeChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const override;

  IsBridgeChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsBridgeChecker);
};

class IsTunnelChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const override;

  IsTunnelChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsTunnelChecker);
};

class IsIslandChecker : public BaseChecker
{
  IsIslandChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsIslandChecker);
};

class IsLandChecker : public BaseChecker
{
  IsLandChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsLandChecker);

  uint32_t GetLandType() const;
};

class IsCoastlineChecker : public BaseChecker
{
  IsCoastlineChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCoastlineChecker);

  uint32_t GetCoastlineType() const;
};

class IsHotelChecker : public BaseChecker
{
public:
  enum class Type
  {
    Hotel,
    Apartment,
    CampSite,
    Chalet,
    GuestHouse,
    Hostel,
    Motel,
    Resort,

    Count
  };

  using UnderlyingType = std::underlying_type_t<Type>;

  static_assert(base::Underlying(Type::Count) <= CHAR_BIT * sizeof(unsigned),
                "Too many types of hotels");

  static char const * GetHotelTypeTag(Type type);

  unsigned GetHotelTypesMask(FeatureType & ft) const;

  std::optional<Type> GetHotelType(FeatureType & ft) const;

  DECLARE_CHECKER_INSTANCE(IsHotelChecker);
private:
  IsHotelChecker();

  std::array<std::pair<uint32_t, Type>, base::Underlying(Type::Count)> m_sortedTypes;
};

class IsBookingHotelChecker : public BaseChecker
{
  IsBookingHotelChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBookingHotelChecker);
};

// WiFi is a type in classificator.txt,
// it should be checked for filling metadata in MapObject.
class IsWifiChecker : public BaseChecker
{
  IsWifiChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsWifiChecker);
};

class IsEatChecker : public BaseChecker
{
public:
  enum class Type
  {
    Cafe,
    Bakery,
    FastFood,
    Restaurant,
    Bar,
    Pub,
    Biergarten,

    Count
  };

  DECLARE_CHECKER_INSTANCE(IsEatChecker);

  Type GetType(uint32_t t) const;

private:
  IsEatChecker();

  std::array<std::pair<uint32_t, Type>, base::Underlying(Type::Count)> m_sortedTypes;
};

class IsCuisineChecker : public BaseChecker
{
  IsCuisineChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCuisineChecker);
};

class IsRecyclingTypeChecker : public BaseChecker
{
  IsRecyclingTypeChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingTypeChecker);
};

class IsCityChecker : public BaseChecker
{
  IsCityChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCityChecker);
};

class IsCapitalChecker : public BaseChecker
{
  IsCapitalChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCapitalChecker);
};

class IsPublicTransportStopChecker : public BaseChecker
{
  IsPublicTransportStopChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsPublicTransportStopChecker);
};

class IsMotorwayJunctionChecker : public BaseChecker
{
  IsMotorwayJunctionChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsMotorwayJunctionChecker);
};

class IsFerryChecker : public BaseChecker
{
  IsFerryChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsFerryChecker);
};

/// Type of locality (do not change values and order - they have detalization order)
/// Country < State < City < ...
enum class LocalityType
{
  None = -1,
  Country = 0,
  State,
  City,
  Town,
  Village,
  Count
};

static_assert(base::Underlying(LocalityType::Country) < base::Underlying(LocalityType::State), "");
static_assert(base::Underlying(LocalityType::State) < base::Underlying(LocalityType::City), "");
static_assert(base::Underlying(LocalityType::City) < base::Underlying(LocalityType::Town), "");
static_assert(base::Underlying(LocalityType::Town) < base::Underlying(LocalityType::Village), "");
static_assert(base::Underlying(LocalityType::Village) < base::Underlying(LocalityType::Count), "");

class IsLocalityChecker : public BaseChecker
{
  IsLocalityChecker();
public:
  LocalityType GetType(uint32_t t) const;
  LocalityType GetType(feature::TypesHolder const & types) const;
  LocalityType GetType(FeatureType & f) const;

  DECLARE_CHECKER_INSTANCE(IsLocalityChecker);
};


class IsCountryChecker : public BaseChecker
{
  IsCountryChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCountryChecker);
};

class IsStateChecker : public BaseChecker
{
  IsStateChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsStateChecker);
};

class IsCityTownOrVillageChecker : public BaseChecker
{
  IsCityTownOrVillageChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCityTownOrVillageChecker);
};

class IsEntranceChecker : public BaseChecker
{
  IsEntranceChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsEntranceChecker);
};

class IsAerowayGateChecker : public BaseChecker
{
  IsAerowayGateChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAerowayGateChecker);
};

class IsRailwaySubwayEntranceChecker : public BaseChecker
{
  IsRailwaySubwayEntranceChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRailwaySubwayEntranceChecker);
};


/// @name Get city radius and population.
/// @param r Radius in meters.
//@{
uint64_t GetPopulation(FeatureType & ft);
double GetRadiusByPopulation(uint64_t p);
double GetRadiusByPopulationForRouting(uint64_t p, LocalityType localityType);
uint64_t GetPopulationByRadius(double r);
//@}

/// Check if type conforms the path. Strings in the path can be
/// feature types like "highway", "living_street", "bridge" and so on
///  or *. * means any class.
/// The root name ("world") is ignored
bool IsTypeConformed(uint32_t type, base::StringIL const & path);

// Highway class. The order is important.
// The enum values follow from the biggest roads (Trunk) to the smallest ones (Service).
enum class HighwayClass
{
  Undefined = 0,  // There has not been any attempt of calculating HighwayClass.
  Error,          // There was an attempt of calculating HighwayClass but it was not successful.
  Trunk,
  Primary,
  Secondary,
  Tertiary,
  LivingStreet,
  Service,
  Pedestrian,
  Transported,    // Vehicles are transported by train or ferry.
  Count           // This value is used for internals only.
};

std::string DebugPrint(HighwayClass const cls);
std::string DebugPrint(LocalityType const localityType);

HighwayClass GetHighwayClass(feature::TypesHolder const & types);
}  // namespace ftypes

namespace std
{
template<>
struct hash<ftypes::IsHotelChecker::Type>
{
  size_t operator()(ftypes::IsHotelChecker::Type type) const
  {
    using UnderlyingType = ftypes::IsHotelChecker::UnderlyingType;
    return hash<UnderlyingType>()(static_cast<UnderlyingType>(type));
  }
};
}  // namespace std
