#pragma once

#include "base/base.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"

namespace feature { class TypesHolder; }
class FeatureType;

namespace ftypes
{

class BaseChecker
{
  size_t const m_level;
  virtual bool IsMatched(uint32_t type) const;

protected:
  vector<uint32_t> m_types;

public:
  BaseChecker(size_t level = 2) : m_level(level) {}
  virtual ~BaseChecker() {}

  bool operator() (feature::TypesHolder const & types) const;
  bool operator() (FeatureType const & ft) const;
  bool operator() (vector<uint32_t> const & types) const;

  static uint32_t PrepareToMatch(uint32_t type, uint8_t level);
};

class IsPeakChecker : public BaseChecker
{
public:
  IsPeakChecker();

  static IsPeakChecker const & Instance();
};

class IsATMChecker : public BaseChecker
{
public:
  IsATMChecker();

  static IsATMChecker const & Instance();
};

class IsSpeedCamChecker : public BaseChecker
{
public:
  IsSpeedCamChecker();

  static IsSpeedCamChecker const & Instance();
};

class IsFuelStationChecker : public BaseChecker
{
public:
  IsFuelStationChecker();

  static IsFuelStationChecker const & Instance();
};

class IsStreetChecker : public BaseChecker
{
public:
  IsStreetChecker();

  static IsStreetChecker const & Instance();
};

class IsOneWayChecker : public BaseChecker
{
public:
  IsOneWayChecker();

  static IsOneWayChecker const & Instance();
};

class IsRoundAboutChecker : public BaseChecker
{
public:
  IsRoundAboutChecker();

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
  virtual bool IsMatched(uint32_t type) const;
public:
  IsBridgeChecker();
  static IsBridgeChecker const & Instance();
};

class IsTunnelChecker : public BaseChecker
{
  virtual bool IsMatched(uint32_t type) const;
public:
  IsTunnelChecker();
  static IsTunnelChecker const & Instance();
};

/// Type of locality (do not change values and order - they have detalization order)
/// COUNTRY < STATE < CITY < ...
enum Type { NONE = -1, COUNTRY = 0, STATE, CITY, TOWN, VILLAGE, LOCALITY_COUNT };

class IsLocalityChecker : public BaseChecker
{
public:
  IsLocalityChecker();

  Type GetType(feature::TypesHolder const & types) const;
  Type GetType(FeatureType const & f) const;

  static IsLocalityChecker const & Instance();
};

/// @name Get city radius and population.
/// @param r Radius in meters.
//@{
uint32_t GetPopulation(FeatureType const & ft);
double GetRadiusByPopulation(uint32_t p);
uint32_t GetPopulationByRadius(double r);

/// Check if type conforms the path. Strings in the path can be
/// feature types like "highway", "living_street", "bridge" and so on
///  or *. * means any class.
/// The root name ("world") is ignored
bool IsTypeConformed(uint32_t type, vector<string> const & path);

// Highway class. The order is important.
// The enum values follow from the biggest roads (Trunk) to the smallest ones (Service).
enum class HighwayClass
{
  Undefined = 0,  // There has not been any attempt of calculating HighwayClass.
  Error,   // There was an attempt of calculating HighwayClass but it was not successful.
  Trunk,
  Primary,
  Secondary,
  Tertiary,
  LivingStreet,
  Service,
  Count  // This value is used for internals only.
};

string DebugPrint(HighwayClass const cls);

HighwayClass GetHighwayClass(feature::TypesHolder const & types);
HighwayClass GetHighwayClass(FeatureType const & ft);

//@}
}  // namespace ftypes
