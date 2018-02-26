#pragma once

#include "indexer/feature_data.hpp"

#include "base/base.hpp"

#include "std/algorithm.hpp"
#include "std/array.hpp"
#include "std/initializer_list.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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
  vector<uint32_t> m_types;

  BaseChecker(size_t level = 2) : m_level(level) {}
  virtual ~BaseChecker() = default;

public:
  virtual bool IsMatched(uint32_t type) const;

  bool operator() (feature::TypesHolder const & types) const;
  bool operator() (FeatureType const & ft) const;
  bool operator() (vector<uint32_t> const & types) const;

  static uint32_t PrepareToMatch(uint32_t type, uint8_t level);

  template <typename TFn>
  void ForEachType(TFn && fn) const
  {
    for_each(m_types.cbegin(), m_types.cend(), forward<TFn>(fn));
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

  static_assert(static_cast<size_t>(Type::Count) <= CHAR_BIT * sizeof(unsigned),
                "Too many types of hotels");

  static char const * GetHotelTypeTag(Type type);

  unsigned GetHotelTypesMask(FeatureType const & ft) const;

  DECLARE_CHECKER_INSTANCE(IsHotelChecker);
private:
  IsHotelChecker();

  array<pair<uint32_t, Type>, static_cast<size_t>(Type::Count)> m_sortedTypes;
};

// WiFi is a type in classificator.txt,
// it should be checked for filling metadata in MapObject.
class IsWifiChecker : public BaseChecker
{
  IsWifiChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsWifiChecker);
};

class IsFoodChecker : public BaseChecker
{
  IsFoodChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsFoodChecker);
};

// Checks for types that are not drawable, but searchable.
class IsInvisibleIndexedChecker : public BaseChecker
{
  IsInvisibleIndexedChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsInvisibleIndexedChecker);
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
  Type GetType(FeatureType const & f) const;

  DECLARE_CHECKER_INSTANCE(IsLocalityChecker);
};

template <typename Types>
bool IsTownOrCity(Types const & types)
{
  feature::TypesHolder h;
  for (auto const t : types)
    h.Add(t);
  auto const type = IsLocalityChecker::Instance().GetType(h);
  return type == TOWN || type == CITY;
}

/// @name Get city radius and population.
/// @param r Radius in meters.
//@{
uint64_t GetPopulation(FeatureType const & ft);
double GetRadiusByPopulation(uint64_t p);
uint64_t GetPopulationByRadius(double r);
//@}

/// Check if type conforms the path. Strings in the path can be
/// feature types like "highway", "living_street", "bridge" and so on
///  or *. * means any class.
/// The root name ("world") is ignored
bool IsTypeConformed(uint32_t type, StringIL const & path);

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
  Transported,    // Vehicles are transported by a train or a ferry.
  Count  // This value is used for internals only.
};

string DebugPrint(HighwayClass const cls);

HighwayClass GetHighwayClass(feature::TypesHolder const & types);
}  // namespace ftypes
