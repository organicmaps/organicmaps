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
  static IsPeakChecker const & Instance();
};

class IsATMChecker : public BaseChecker
{
  IsATMChecker();
public:
  static IsATMChecker const & Instance();
};

class IsSpeedCamChecker : public BaseChecker
{
  IsSpeedCamChecker();
public:
  static IsSpeedCamChecker const & Instance();
};

class IsFuelStationChecker : public BaseChecker
{
  IsFuelStationChecker();
public:
  static IsFuelStationChecker const & Instance();
};

class IsRailwayStationChecker : public BaseChecker
{
  IsRailwayStationChecker();
public:
  static IsRailwayStationChecker const & Instance();
};

class IsStreetChecker : public BaseChecker
{
  IsStreetChecker();
public:
  static IsStreetChecker const & Instance();
};

class IsAddressObjectChecker : public BaseChecker
{
  IsAddressObjectChecker();
public:
  static IsAddressObjectChecker const & Instance();
};

class IsVillageChecker : public BaseChecker
{
  IsVillageChecker();

public:
  static IsVillageChecker const & Instance();
};

class IsOneWayChecker : public BaseChecker
{
  IsOneWayChecker();
public:
  static IsOneWayChecker const & Instance();
};

class IsRoundAboutChecker : public BaseChecker
{
  IsRoundAboutChecker();
public:
  static IsRoundAboutChecker const & Instance();
};

class IsLinkChecker : public BaseChecker
{
  IsLinkChecker();
public:
  static IsLinkChecker const & Instance();
};

class IsBuildingChecker : public BaseChecker
{
  IsBuildingChecker();
public:
  static IsBuildingChecker const & Instance();
  uint32_t GetMainType() const { return m_types[0]; }
};

class IsBridgeChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const override;

  IsBridgeChecker();
public:
  static IsBridgeChecker const & Instance();
};

class IsTunnelChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const override;

  IsTunnelChecker();
public:
  static IsTunnelChecker const & Instance();
};

class IsBookingChecker : public BaseChecker
{
  IsBookingChecker();
public:
  static IsBookingChecker const & Instance();
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

  static IsHotelChecker const & Instance();

  static char const * GetHotelTypeTag(Type type);

  unsigned GetHotelTypesMask(FeatureType const & ft) const;

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
  static IsWifiChecker const & Instance();
};

class IsFoodChecker : public BaseChecker
{
  IsFoodChecker();
public:
  static IsFoodChecker const & Instance();
};

class IsOpentableChecker : public BaseChecker
{
  IsOpentableChecker();

public:
  static IsOpentableChecker const & Instance();
};

// Checks for types that are not drawable, but searchable.
class IsInvisibleIndexedChecker : public BaseChecker
{
  IsInvisibleIndexedChecker();

public:
  static IsInvisibleIndexedChecker const & Instance();
};

class IsCityChecker : public BaseChecker
{
  IsCityChecker();

public:
  static IsCityChecker const & Instance();
};

class IsViatorChecker : public BaseChecker
{
  IsViatorChecker();

public:
  static IsViatorChecker const & Instance();
};

class IsThorChecker : public BaseChecker
{
  IsThorChecker();

public:
  static IsThorChecker const & Instance();
};

class IsPublicTransportStopChecker : public BaseChecker
{
  IsPublicTransportStopChecker();

public:
  static IsPublicTransportStopChecker const & Instance();
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

  static IsLocalityChecker const & Instance();
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
  Count  // This value is used for internals only.
};

string DebugPrint(HighwayClass const cls);

HighwayClass GetHighwayClass(feature::TypesHolder const & types);
}  // namespace ftypes
