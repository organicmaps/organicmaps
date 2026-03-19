#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"

#include "base/small_map.hpp"
#include "base/stl_helpers.hpp"

#include <string>
#include <vector>

#define DECLARE_CHECKER_INSTANCE(CheckerType) \
  static CheckerType const & Instance()       \
  {                                           \
    static CheckerType const inst;            \
    return inst;                              \
  }

/* If fancy post-initialization needed.
#define DECLARE_CHECKER_INSTANCE(CheckerType) \
  static CheckerType const & Instance()       \
  {                                           \
    struct Initializer : public CheckerType   \
    {                                         \
      Initializer() { PostInitialize(); }     \
    };                                        \
    static Initializer const inst;            \
    return inst;                              \
  }
*/

namespace ftypes
{
/// Use only for non-trivial matching logic. @see BaseCheckerEx otherwise.
class BaseChecker
{
protected:
  uint8_t const m_level;
  std::vector<uint32_t> m_types;

  explicit BaseChecker(uint8_t level = 2) : m_level(level) {}
  virtual ~BaseChecker() = default;

public:
  void PostInitialize();

  virtual bool IsMatched(uint32_t type) const;

  template <class FnT>
  void ForEachType(FnT && fn) const
  {
    for (auto const & t : m_types)
      fn(t);
  }

  std::vector<uint32_t> const & GetTypes() const { return m_types; }

  bool operator()(FeatureType & ft) const;
  bool operator()(uint32_t type) const { return IsMatched(type); }
  template <class T>
  bool operator()(T && types) const
  {
    for (uint32_t t : types)
      if (IsMatched(t))
        return true;
    return false;
  }

  static uint32_t PrepareToMatch(uint32_t type, uint8_t level);
};

/// @brief Simple base checker class for matching with subclasses.
/// Consider using it first when adding new checker.
class BaseCheckerEx
{
protected:
  std::vector<std::pair<uint32_t, uint8_t>> m_types;

public:
  BaseCheckerEx(std::initializer_list<base::StringIL> const & lst);
  void PostInitialize() {}

  template <class FnT>
  void ForEachType(FnT && fn) const
  {
    for (auto const & t : m_types)
      fn(t.first);
  }

  bool operator()(uint32_t type) const
  {
    for (auto const & e : m_types)
      if (e.first == ftype::Trunc(type, e.second))
        return true;
    return false;
  }

  template <class T>
  bool operator()(T && types) const
  {
    for (uint32_t t : types)
      if (this->operator()(t))
        return true;
    return false;
  }

  bool operator()(FeatureType & ft) const;
};

class IsPeakChecker : public BaseCheckerEx
{
  IsPeakChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPeakChecker);
};

class IsATMChecker : public BaseCheckerEx
{
  IsATMChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsATMChecker);
};

class IsSpeedCamChecker : public BaseCheckerEx
{
  IsSpeedCamChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSpeedCamChecker);
};

class IsPostBoxChecker : public BaseCheckerEx
{
  IsPostBoxChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPostBoxChecker);
};

class IsPostPoiChecker : public BaseChecker
{
  IsPostPoiChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPostPoiChecker);
};

class IsOperatorOthersPoiChecker : public BaseChecker
{
  IsOperatorOthersPoiChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsOperatorOthersPoiChecker);
};

class IsRecyclingCentreChecker : public BaseCheckerEx
{
  IsRecyclingCentreChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingCentreChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class IsRecyclingContainerChecker : public BaseChecker
{
  IsRecyclingContainerChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingContainerChecker);

  uint32_t GetType() const { return m_types[0]; }
};

class IsRailwayStationChecker : public BaseChecker
{
  IsRailwayStationChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRailwayStationChecker);
};

class IsSubwayStationChecker : public BaseCheckerEx
{
  IsSubwayStationChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSubwayStationChecker);
};

class IsAirportChecker : public BaseCheckerEx
{
  IsAirportChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAirportChecker);
};

struct IsSquareChecker : public BaseCheckerEx
{
  IsSquareChecker();
  DECLARE_CHECKER_INSTANCE(IsSquareChecker);
};

// Type of suburb.
// Do not change values and order - they are in the order of decreasing specificity.
// Suburb > Neighbourhood > Residential
enum class SuburbType
{
  Residential = 0,
  Neighbourhood,
  Quarter,
  Suburb,
  Count
};

class IsSuburbChecker : public BaseChecker
{
  IsSuburbChecker();

public:
  SuburbType GetType(uint32_t t) const;
  SuburbType GetType(feature::TypesHolder const & types) const;
  SuburbType GetType(FeatureType & f) const;

  DECLARE_CHECKER_INSTANCE(IsSuburbChecker);
};

/// @todo Better to rename like IsStreetChecker, as it is used in search context only?
class IsWayChecker : public BaseChecker
{
public:
  IsWayChecker();
  DECLARE_CHECKER_INSTANCE(IsWayChecker);

  enum SearchRank : uint8_t
  {
    Default = 0,  // Not a road, other linear way like river, rail ...

    // Bigger is better (more important).
    Pedestrian,
    Cycleway,
    Outdoor,
    Minors,
    Residential,
    Regular,
    Square,  /// @see IsStreetOrSquareChecker
    Motorway,

    Count
  };

  SearchRank GetSearchRank(uint32_t type) const;

private:
  base::SmallMap<uint32_t, SearchRank> m_ranks;
};

class IsStreetOrSquareChecker
{
public:
  template <class T>
  bool operator()(T && t) const
  {
    return m_street(t) || m_square(t);
  }

  // Additional checks for the appropriate geometry type.
  bool operator()(FeatureType & ft) const;

  template <class FnT>
  void ForEachType(FnT && fn) const
  {
    m_street.ForEachType(fn);
    m_square.ForEachType(fn);
  }

  DECLARE_CHECKER_INSTANCE(IsStreetOrSquareChecker);

  IsWayChecker::SearchRank GetSearchRank(uint32_t type) const;

private:
  IsWayChecker m_street;
  IsSquareChecker m_square;
};

class IsAddressObjectChecker
{
public:
  template <class T>
  bool operator()(T && t) const
  {
    return m_oneLevel(t) || m_twoLevel(t);
  }

  DECLARE_CHECKER_INSTANCE(IsAddressObjectChecker);

private:
  struct AddressOneLevel : BaseChecker
  {
    AddressOneLevel();
  } m_oneLevel;
  struct AddressTwoLevel : BaseChecker
  {
    AddressTwoLevel();
  } m_twoLevel;
};

class IsAddressChecker : public BaseChecker
{
  IsAddressChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAddressChecker);
};

class IsVillageChecker : public BaseChecker
{
  IsVillageChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsVillageChecker);
};

class IsOneWayChecker : public BaseCheckerEx
{
  IsOneWayChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsOneWayChecker);

  uint32_t GetType() const { return m_types[0].first; }
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

class IsBuildingChecker : public BaseCheckerEx
{
public:
  IsBuildingChecker();

  DECLARE_CHECKER_INSTANCE(IsBuildingChecker);
};

class IsBuildingPartChecker : public BaseCheckerEx
{
  IsBuildingPartChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBuildingPartChecker);
};

class IsBuildingHasPartsChecker : public BaseCheckerEx
{
  IsBuildingHasPartsChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBuildingHasPartsChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class IsIsolineChecker : public BaseCheckerEx
{
  IsIsolineChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsIsolineChecker);
};

class IsPisteChecker : public BaseCheckerEx
{
  IsPisteChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPisteChecker);
};

class IsMwmBorderChecker : public ftypes::BaseCheckerEx
{
  IsMwmBorderChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsMwmBorderChecker);
};

class OneLevelPOIChecker : public ftypes::BaseChecker
{
public:
  OneLevelPOIChecker();
};

/// Describes 2-level POI-exception types that don't belong to any POI-common classes
/// (amenity, shop, tourism, ...). Used in search algo and search categories index generation.
class TwoLevelPOIChecker : public ftypes::BaseChecker
{
public:
  TwoLevelPOIChecker();
};

// Used in:
// - search ranking (POIs are ranked somewhat higher than "unclassified" features, see search/model.hpp)
// - add type's translation into complex PP subtitle (see GetLocalizedAllTypes())
// - building lists of popular places (generator/popular_places_section_builder.cpp)
class IsPoiChecker
{
public:
  DECLARE_CHECKER_INSTANCE(IsPoiChecker);

  template <class T>
  bool operator()(T && t) const
  {
    return m_oneLevel(t) || m_twoLevel(t);
  }

private:
  OneLevelPOIChecker m_oneLevel;
  TwoLevelPOIChecker m_twoLevel;
};

class IsAmenityChecker : public BaseCheckerEx
{
  IsAmenityChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAmenityChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class AttractionsChecker : public BaseChecker
{
  size_t m_additionalTypesStart;

  AttractionsChecker();

public:
  DECLARE_CHECKER_INSTANCE(AttractionsChecker);

  // Used in generator.
  uint32_t GetBestType(FeatureParams::Types const & types) const;
};

class IsPlaceChecker : public BaseCheckerEx
{
  IsPlaceChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPlaceChecker);
};

class IsBridgeOrTunnelChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const override;

public:
  IsBridgeOrTunnelChecker();
};

class IsIslandChecker : public BaseChecker
{
  IsIslandChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsIslandChecker);
};

class IsLandChecker : public BaseCheckerEx
{
  IsLandChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsLandChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class IsCoastlineChecker : public BaseCheckerEx
{
  IsCoastlineChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCoastlineChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class IsHotelChecker : public BaseChecker
{
  IsHotelChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsHotelChecker);
};

class IsCampPitchChecker : public BaseCheckerEx
{
  IsCampPitchChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCampPitchChecker);
};

// WiFi is a type in classificator.txt,
// it should be checked for filling metadata in MapObject.
class IsWifiChecker : public BaseCheckerEx
{
  IsWifiChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsWifiChecker);

  uint32_t GetType() const { return m_types[0].first; }
};

class IsEatChecker : public BaseChecker
{
public:
  //  enum class Type
  //  {
  //    Cafe = 0,
  //    FastFood,
  //    Restaurant,
  //    Bar,
  //    Pub,
  //    Biergarten,

  //    Count
  //  };

  DECLARE_CHECKER_INSTANCE(IsEatChecker);

  //  Type GetType(uint32_t t) const;

private:
  IsEatChecker();

  //  std::array<uint32_t, base::Underlying(Type::Count)> m_eat2clType;
};

class IsCuisineChecker : public BaseCheckerEx
{
  IsCuisineChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCuisineChecker);
};

class IsRecyclingTypeChecker : public BaseCheckerEx
{
  IsRecyclingTypeChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsRecyclingTypeChecker);
};

class IsFeeTypeChecker : public BaseCheckerEx
{
  IsFeeTypeChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsFeeTypeChecker);
};

class IsToiletsChecker : public BaseChecker
{
  IsToiletsChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsToiletsChecker);
};

class IsCapitalChecker : public BaseCheckerEx
{
  IsCapitalChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCapitalChecker);
};

class IsParkingChecker : public BaseCheckerEx
{
  IsParkingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsParkingChecker);
};

class IsCarChargingChecker : public BaseCheckerEx
{
  IsCarChargingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCarChargingChecker);
};

class IsBicycleParkingChecker : public BaseChecker
{
  IsBicycleParkingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBicycleParkingChecker);
};

class IsBicycleChargingChecker : public BaseCheckerEx
{
  IsBicycleChargingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsBicycleChargingChecker);
};

class IsMotorcycleParkingChecker : public BaseCheckerEx
{
  IsMotorcycleParkingChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsMotorcycleParkingChecker);
};

class IsPublicTransportStopChecker : public BaseChecker
{
  IsPublicTransportStopChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPublicTransportStopChecker);
};

class IsTaxiChecker : public BaseCheckerEx
{
  IsTaxiChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsTaxiChecker);
};

class IsMotorwayJunctionChecker : public BaseCheckerEx
{
  IsMotorwayJunctionChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsMotorwayJunctionChecker);
};

/// Exotic OSM ways that potentially have "duration" tag.
class IsWayWithDurationChecker : public BaseChecker
{
  IsWayWithDurationChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsWayWithDurationChecker);
};

/// Type of locality (do not change values and order - they have detalization order)
/// Country < State < City < ...
enum class LocalityType : int8_t
{
  None = -1,
  Country = 0,
  State,
  City,
  Town,
  Village,
  Count
};

LocalityType LocalityFromString(std::string_view s);

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

  template <class Types>
  LocalityType GetType(Types const & types) const
  {
    for (uint32_t const t : types)
    {
      LocalityType const type = GetType(t);
      if (type != LocalityType::None)
        return type;
    }
    return LocalityType::None;
  }

  LocalityType GetType(FeatureType & f) const;

  DECLARE_CHECKER_INSTANCE(IsLocalityChecker);
};

class IsCountryChecker : public BaseCheckerEx
{
  IsCountryChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCountryChecker);
};

class IsStateChecker : public BaseCheckerEx
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

class IsEntranceChecker : public BaseCheckerEx
{
  IsEntranceChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsEntranceChecker);
};

class IsAerowayGateChecker : public BaseCheckerEx
{
  IsAerowayGateChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsAerowayGateChecker);
};

class IsSubwayEntranceChecker : public BaseCheckerEx
{
  IsSubwayEntranceChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsSubwayEntranceChecker);
};

class IsPlatformChecker : public BaseChecker
{
  IsPlatformChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsPlatformChecker);
};

class IsAddressInterpolChecker : public BaseChecker
{
  uint32_t m_odd, m_even;

public:
  IsAddressInterpolChecker();

  DECLARE_CHECKER_INSTANCE(IsAddressInterpolChecker);

  template <class Range>
  feature::InterpolType GetInterpolType(Range const & range) const
  {
    for (uint32_t t : range)
    {
      if (t == m_odd)
        return feature::InterpolType::Odd;
      if (t == m_even)
        return feature::InterpolType::Even;

      ftype::TruncValue(t, 1);
      if (t == m_types[0])
        return feature::InterpolType::Any;
    }

    return feature::InterpolType::None;
  }

  feature::InterpolType GetInterpolType(FeatureType & ft) const { return GetInterpolType(feature::TypesHolder(ft)); }
};

/// @name Get city radius and population.
/// @param r Radius in meters.
//@{
uint64_t GetDefPopulation(LocalityType localityType);
uint64_t GetPopulation(FeatureType & ft);
double GetRadiusByPopulation(uint64_t p);
double GetRadiusByPopulationForRouting(uint64_t p, LocalityType localityType);
uint64_t GetPopulationByRadius(double r);
//@}

// Highway class. The order is important.
// The enum values follow from the biggest roads (Trunk) to the smallest ones (Service).
enum class HighwayClass
{
  Undefined = 0,  // There has not been any attempt of calculating HighwayClass.
  Trunk,
  Primary,
  Secondary,
  Tertiary,
  LivingStreet,
  Service,
  // OSM highway=service type is widely used even for _significant_ roads.
  // Adding a new type to distinguish mapped driveway or parking_aisle.
  ServiceMinor,
  Pedestrian,
  Transported,  // Vehicles are transported by train or ferry.
  Count         // This value is used for internals only.
};

std::string DebugPrint(HighwayClass const cls);
std::string DebugPrint(LocalityType const localityType);

HighwayClass GetHighwayClass(feature::TypesHolder const & types);
}  // namespace ftypes
