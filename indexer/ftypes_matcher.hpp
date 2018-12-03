#pragma once

#include "indexer/feature_data.hpp"

#include "base/base.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace feature { class TypesHolder; }
class FeatureType;

#define DECLARE_CHECKER_INSTANCE(CheckerType) static CheckerType const & Instance() { \
                                              static CheckerType const inst; return inst; }

namespace ftypes
{
class BaseChecker
{
  size_t const m_level;

protected:
  std::vector<uint32_t> m_types;

  BaseChecker(size_t level = 2) : m_level(level) {}
  virtual ~BaseChecker() = default;

public:
  virtual bool IsMatched(uint32_t type) const;

  bool operator()(feature::TypesHolder const & types) const;
  bool operator()(FeatureType & ft) const;
  bool operator()(std::vector<uint32_t> const & types) const;
  bool operator()(uint32_t type) const { return IsMatched(type); }

  static uint32_t PrepareToMatch(uint32_t type, uint8_t level);

  template <typename TFn>
  void ForEachType(TFn && fn) const
  {
    std::for_each(m_types.cbegin(), m_types.cend(), std::forward<TFn>(fn));
  }
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

class IsSpeedCamChecker : public BaseChecker
{
  IsSpeedCamChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsSpeedCamChecker);
};

class IsFuelStationChecker : public BaseChecker
{
  IsFuelStationChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsFuelStationChecker);
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

class IsStreetChecker : public BaseChecker
{
  IsStreetChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsStreetChecker);
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

class IsPoiChecker : public BaseChecker
{
  IsPoiChecker();
public:
  static std::set<std::string> const kPoiTypes;

  DECLARE_CHECKER_INSTANCE(IsPoiChecker);
};

class WikiChecker : public BaseChecker
{
  WikiChecker();
public:
   static std::set<std::pair<std::string, std::string>> const kTypesForWiki;

   DECLARE_CHECKER_INSTANCE(WikiChecker);

   template <typename Ft>
   bool NeedFeature(Ft & feature) const
   {
     bool need = true;
     feature.ForEachType([&](uint32_t type) {
       if (need && !IsMatched(type))
         need = false;
     });
     return need;
   }
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

class IsPopularityPlaceChecker : public BaseChecker
{
  IsPopularityPlaceChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsPopularityPlaceChecker);
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

  static_assert(base::Key(Type::Count) <= CHAR_BIT * sizeof(unsigned),
                "Too many types of hotels");

  static char const * GetHotelTypeTag(Type type);

  unsigned GetHotelTypesMask(FeatureType & ft) const;

  boost::optional<Type> GetHotelType(FeatureType & ft) const;

  DECLARE_CHECKER_INSTANCE(IsHotelChecker);
private:
  IsHotelChecker();

  std::array<std::pair<uint32_t, Type>, base::Key(Type::Count)> m_sortedTypes;
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

  std::vector<uint32_t> const & GetTypes() const { return m_types; }
  Type GetType(uint32_t t) const;

private:
  IsEatChecker();

  std::array<std::pair<uint32_t, Type>, base::Key(Type::Count)> m_sortedTypes;
};

class IsCuisineChecker : public BaseChecker
{
  IsCuisineChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsCuisineChecker);
};

class IsCityChecker : public BaseChecker
{
  IsCityChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsCityChecker);
};

class IsPublicTransportStopChecker : public BaseChecker
{
  IsPublicTransportStopChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsPublicTransportStopChecker);
};

/// Type of locality (do not change values and order - they have detalization order)
/// COUNTRY < STATE < CITY < ...
enum Type { NONE = -1, COUNTRY = 0, STATE, CITY, TOWN, VILLAGE, LOCALITY_COUNT };

class IsLocalityChecker : public BaseChecker
{
  IsLocalityChecker();
public:
  Type GetType(uint32_t t) const;
  Type GetType(feature::TypesHolder const & types) const;
  Type GetType(FeatureType & f) const;

  DECLARE_CHECKER_INSTANCE(IsLocalityChecker);
};

template <typename Types>
bool IsCityTownOrVillage(Types const & types)
{
  feature::TypesHolder h;
  for (auto const t : types)
    h.Add(t);
  auto const type = IsLocalityChecker::Instance().GetType(h);
  return type == CITY || type == TOWN || type == VILLAGE;
}

/// @name Get city radius and population.
/// @param r Radius in meters.
//@{
uint64_t GetPopulation(FeatureType & ft);
double GetRadiusByPopulation(uint64_t p);
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
