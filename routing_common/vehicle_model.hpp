#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "base/stl_helpers.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Classificator;
class FeatureType;

namespace feature { class TypesHolder; }

namespace routing
{
double constexpr kNotUsed = std::numeric_limits<double>::max();

struct InOutCityFactor;
struct InOutCitySpeedKMpH;

// Each value is equal to the corresponding type index from types.txt.
// The ascending order is strict.
enum class HighwayType : uint32_t
{
  HighwayResidential = 1,
  HighwayService = 2,
  HighwayUnclassified = 4,
  HighwayFootway = 6,
  HighwayTrack = 7,
  HighwayTertiary = 8,
  HighwaySecondary = 12,
  HighwayPath = 15,
  HighwayPrimary = 26,
  HighwayRoad = 30,
  HighwayCycleway = 36,
  HighwayMotorwayLink = 43,
  HighwayLivingStreet = 54,
  HighwayMotorway = 57,
  HighwaySteps = 58,
  HighwayTrunk = 65,
  HighwayPedestrian = 69,
  HighwayTrunkLink = 90,
  HighwayPrimaryLink = 95,
  ManMadePier = 119,
  HighwayBridleway = 167,
  HighwaySecondaryLink = 176,
  RouteFerry = 259,
  HighwayTertiaryLink = 272,
  RouteFerryMotorcar = 988,
  RouteFerryMotorVehicle = 993,
  RailwayRailMotorVehicle = 994,
  RouteShuttleTrain = 1054,
};

using HighwayBasedFactors = std::unordered_map<HighwayType, InOutCityFactor, base::EnumClassHash>;
using HighwayBasedSpeeds = std::unordered_map<HighwayType, InOutCitySpeedKMpH, base::EnumClassHash>;

/// \brief Params for calculation of an approximate speed on a feature.
struct SpeedParams
{
  SpeedParams(bool forward, bool inCity, Maxspeed maxspeed)
    : m_forward(forward), m_inCity(inCity), m_maxspeed(std::move(maxspeed))
  {
  }

  bool m_forward;
  // |m_inCity| == true if a corresponding feature lies inside a city of a town.
  bool m_inCity;
  Maxspeed m_maxspeed;
};

/// \brief Speeds which are used for edge weight and ETA estimations.
struct SpeedKMpH
{
  constexpr SpeedKMpH() = default;
  constexpr SpeedKMpH(double weight) noexcept : m_weight(weight), m_eta(weight) {}
  constexpr SpeedKMpH(double weight, double eta) noexcept : m_weight(weight), m_eta(eta) {}

  bool operator==(SpeedKMpH const & rhs) const
  {
    return m_weight == rhs.m_weight && m_eta == rhs.m_eta;
  }
  bool operator!=(SpeedKMpH const & rhs) const { return !(*this == rhs); }

  bool IsValid() const { return m_weight > 0 && m_eta > 0; }

  double m_weight = 0.0;  // KMpH
  double m_eta = 0.0;     // KMpH
};

/// \brief Factors which modify weight and ETA speed on feature in case of bad pavement (reduce)
/// or good highway class (increase).
/// Both should be greater then 0.
struct SpeedFactor
{
  constexpr SpeedFactor() = default;
  constexpr SpeedFactor(double factor) noexcept : m_weight(factor), m_eta(factor) {}
  constexpr SpeedFactor(double weight, double eta) noexcept : m_weight(weight), m_eta(eta) {}

  bool IsValid() const { return m_weight > 0.0 && m_eta > 0.0; }

  bool operator==(SpeedFactor const & rhs) const
  {
    return m_weight == rhs.m_weight && m_eta == rhs.m_eta;
  }
  bool operator!=(SpeedFactor const & rhs) const { return !(*this == rhs); }

  double m_weight = 1.0;
  double m_eta = 1.0;
};

inline SpeedKMpH operator*(SpeedFactor const & factor, SpeedKMpH const & speed)
{
  return {speed.m_weight * factor.m_weight, speed.m_eta * factor.m_eta};
}

inline SpeedKMpH operator*(SpeedKMpH const & speed, SpeedFactor const & factor)
{
  return {speed.m_weight * factor.m_weight, speed.m_eta * factor.m_eta};
}

struct InOutCitySpeedKMpH
{
  constexpr InOutCitySpeedKMpH() = default;
  constexpr explicit InOutCitySpeedKMpH(SpeedKMpH const & speed) noexcept
    : m_inCity(speed), m_outCity(speed)
  {
  }
  constexpr InOutCitySpeedKMpH(SpeedKMpH const & inCity, SpeedKMpH const & outCity) noexcept
    : m_inCity(inCity), m_outCity(outCity)
  {
  }

  bool operator==(InOutCitySpeedKMpH const & rhs) const
  {
    return m_inCity == rhs.m_inCity && m_outCity == rhs.m_outCity;
  }

  SpeedKMpH const & GetSpeed(bool isCity) const { return isCity ? m_inCity : m_outCity; }
  bool IsValid() const { return m_inCity.IsValid() && m_outCity.IsValid(); }

  SpeedKMpH m_inCity;
  SpeedKMpH m_outCity;
};

struct InOutCityFactor
{
  constexpr explicit InOutCityFactor(SpeedFactor const & factor) noexcept
    : m_inCity(factor), m_outCity(factor)
  {
  }
  constexpr InOutCityFactor(SpeedFactor const & inCity, SpeedFactor const & outCity) noexcept
    : m_inCity(inCity), m_outCity(outCity)
  {
  }

  bool operator==(InOutCityFactor const & rhs) const
  {
    return m_inCity == rhs.m_inCity && m_outCity == rhs.m_outCity;
  }

  SpeedFactor const & GetFactor(bool isCity) const { return isCity ? m_inCity : m_outCity; }
  bool IsValid() const { return m_inCity.IsValid() && m_outCity.IsValid(); }

  SpeedFactor m_inCity;
  SpeedFactor m_outCity;
};

struct HighwayBasedInfo
{
  HighwayBasedInfo(HighwayBasedSpeeds const & speeds, HighwayBasedFactors const & factors)
    : m_speeds(speeds)
    , m_factors(factors)
  {
  }

  HighwayBasedSpeeds const & m_speeds;
  HighwayBasedFactors const & m_factors;
};

class VehicleModelInterface
{
public:
  enum class RoadAvailability
  {
    NotAvailable,
    Available,
    Unknown,
  };

  virtual ~VehicleModelInterface() = default;

  /// @return Allowed weight and ETA speed in KMpH.
  /// 0 means that it's forbidden to move on this feature or it's not a road at all.
  /// Weight and ETA should be less than max model speed's values respectively.
  /// @param inCity is true if |f| lies in a city of town.
  virtual SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const = 0;

  virtual HighwayType GetHighwayType(FeatureType & f) const = 0;

  /// @return Maximum model weight speed.
  /// All speeds which the model returns must be less than or equal to this speed.
  virtual double GetMaxWeightSpeed() const = 0;

  /// @return Offroad speed in KMpH for vehicle. This speed should be used for non-feature routing
  /// e.g. to connect start point to nearest feature.
  virtual SpeedKMpH const & GetOffroadSpeed() const = 0;

  virtual bool IsOneWay(FeatureType & f) const = 0;

  /// @returns true iff feature |f| can be used for routing with corresponding vehicle model.
  virtual bool IsRoad(FeatureType & f) const = 0;

  /// @returns true iff feature |f| can be used for through passage with corresponding vehicle model.
  /// e.g. in Russia roads tagged "highway = service" are not allowed for through passage;
  /// however, road with this tag can be be used if it is close enough to the start or destination
  /// point of the route.
  /// Roads with additional types e.g. "path = ferry", "vehicle_type = yes" considered as allowed
  /// to pass through.
  virtual bool IsPassThroughAllowed(FeatureType & f) const = 0;
};

class VehicleModelFactoryInterface
{
public:
  virtual ~VehicleModelFactoryInterface() = default;
  /// @return Default vehicle model which corresponds for all countrines,
  /// but it may be non optimal for some countries
  virtual std::shared_ptr<VehicleModelInterface> GetVehicleModel() const = 0;

  /// @return The most optimal vehicle model for specified country
  virtual std::shared_ptr<VehicleModelInterface> GetVehicleModelForCountry(
      std::string const & country) const = 0;
};

class VehicleModel : public VehicleModelInterface
{
public:
  struct FeatureTypeLimits final
  {
    FeatureTypeLimits(std::vector<std::string> const & types, bool isPassThroughAllowed)
      : m_types(types), m_isPassThroughAllowed(isPassThroughAllowed)
    {
    }

    std::vector<std::string> m_types;
    bool m_isPassThroughAllowed;  // Pass through this road type is allowed.
  };

  // Structure for keeping surface tags: psurface|paved_good, psurface|paved_bad,
  // psurface|unpaved_good and psurface|unpaved_bad.
  struct FeatureTypeSurface
  {
    std::vector<std::string> m_types;  // 2-arity road type
    SpeedFactor m_factor;
  };

  struct AdditionalRoadTags final
  {
    AdditionalRoadTags() = default;

    AdditionalRoadTags(std::initializer_list<char const *> const & hwtag,
                       InOutCitySpeedKMpH const & speed)
      : m_hwtag(hwtag.begin(), hwtag.end()), m_speed(speed)
    {
    }

    std::vector<std::string> m_hwtag;
    InOutCitySpeedKMpH m_speed;
  };

  using LimitsInitList = std::initializer_list<FeatureTypeLimits>;
  using SurfaceInitList = std::initializer_list<FeatureTypeSurface>;

  VehicleModel(Classificator const & c, LimitsInitList const & featureTypeLimits,
               SurfaceInitList const & featureTypeSurface, HighwayBasedInfo const & info);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const override;
  HighwayType GetHighwayType(FeatureType & f) const override;
  double GetMaxWeightSpeed() const override;
  bool IsOneWay(FeatureType & f) const override;
  bool IsRoad(FeatureType & f) const override;
  bool IsPassThroughAllowed(FeatureType & f) const override;

public:
  /// @returns true if |m_highwayTypes| or |m_addRoadTypes| contains |type| and false otherwise.
  bool IsRoadType(uint32_t type) const;

  template <class TList>
  bool HasRoadType(TList const & types) const
  {
    for (uint32_t t : types)
    {
      if (IsRoadType(t))
        return true;
    }
    return false;
  }

  bool EqualsForTests(VehicleModel const & rhs) const
  {
    return (m_roadTypes == rhs.m_roadTypes) && (m_addRoadTypes == rhs.m_addRoadTypes) &&
           (m_onewayType == rhs.m_onewayType);
  }

protected:
  /// @returns a special restriction which is set to the feature.
  virtual RoadAvailability GetRoadAvailability(feature::TypesHolder const & types) const;

  /// Used in derived class constructors only. Not for public use.
  void SetAdditionalRoadTypes(Classificator const & c,
                              std::vector<AdditionalRoadTags> const & additionalTags);

  /// \returns true if |types| is a oneway feature.
  /// \note According to OSM, tag "oneway" could have value "-1". That means it's a oneway feature
  /// with reversed geometry. In that case at map generation the geometry of such features
  /// is reversed (the order of points is changed) so in vehicle model all oneway feature
  /// may be considered as features with forward geometry.
  bool HasOneWayType(feature::TypesHolder const & types) const;

  bool HasPassThroughType(feature::TypesHolder const & types) const;

  SpeedKMpH GetTypeSpeed(feature::TypesHolder const & types, SpeedParams const & speedParams) const;

  SpeedKMpH GetSpeedWihtoutMaxspeed(FeatureType & f, SpeedParams const & speedParams) const;

  /// \brief maximum within all the speed limits set in a model (car model, bicycle model and so on).
  /// It shouldn't be mixed with maxspeed value tag which defines maximum legal speed on a feature.
  InOutCitySpeedKMpH m_maxModelSpeed;

private:
  struct AdditionalRoadType final
  {
    AdditionalRoadType(Classificator const & c, AdditionalRoadTags const & tag);

    bool operator==(AdditionalRoadType const & rhs) const { return m_type == rhs.m_type; }

    uint32_t const m_type;
    InOutCitySpeedKMpH const m_speed;
  };

  class RoadType final
  {
  public:
    RoadType(HighwayType hwtype, bool isPassThroughAllowed)
      : m_highwayType(hwtype), m_isPassThroughAllowed(isPassThroughAllowed)
    {
    }

    bool IsPassThroughAllowed() const { return m_isPassThroughAllowed; };
    HighwayType GetHighwayType() const { return m_highwayType; }
    bool operator==(RoadType const & rhs) const
    {
      return m_highwayType == rhs.m_highwayType &&
             m_isPassThroughAllowed == rhs.m_isPassThroughAllowed;
    }

  private:
    HighwayType const m_highwayType;
    bool const m_isPassThroughAllowed;
  };

  struct TypeFactor
  {
    uint32_t m_type = 0;
    SpeedFactor m_factor;
  };

  std::optional<HighwayType> GetHighwayType(uint32_t type) const;
  void GetSurfaceFactor(uint32_t type, SpeedFactor & factor) const;
  void GetAdditionalRoadSpeed(uint32_t type, bool isCityRoad,
                              std::optional<SpeedKMpH> & speed) const;

  SpeedKMpH GetSpeedOnFeatureWithoutMaxspeed(HighwayType const & type,
                                             SpeedParams const & speedParams) const;
  SpeedKMpH GetSpeedOnFeatureWithMaxspeed(HighwayType const & type,
                                          SpeedParams const & speedParams) const;

  std::vector<AdditionalRoadType>::const_iterator FindAdditionalRoadType(uint32_t type) const;

  std::unordered_map<uint32_t, RoadType> m_roadTypes;
  // Mapping surface types (psurface|paved_good, psurface|paved_bad, psurface|unpaved_good,
  // psurface|unpaved_bad) to surface speed factors.
  // Note. It's an array (not map or unordered_map) because of perfomance reasons.
  std::array<TypeFactor, 4> m_surfaceFactors;

  std::vector<AdditionalRoadType> m_addRoadTypes;
  uint32_t m_onewayType;

  HighwayBasedInfo const m_highwayBasedInfo;
};

class VehicleModelFactory : public VehicleModelFactoryInterface
{
public:
  // Callback which takes territory name (mwm graph node name) and returns its parent
  // territory name. Used to properly find local restrictions in GetVehicleModelForCountry.
  // For territories which do not have parent (countries) or have multiple parents
  // (disputed territories) it should return empty name.
  // For example "Munich" -> "Bavaria"; "Bavaria" -> "Germany"; "Germany" -> ""
  using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

  std::shared_ptr<VehicleModelInterface> GetVehicleModel() const override;

  std::shared_ptr<VehicleModelInterface> GetVehicleModelForCountry(
      std::string const & country) const override;

protected:
  explicit VehicleModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn);
  std::string GetParent(std::string const & country) const;

  std::unordered_map<std::string, std::shared_ptr<VehicleModelInterface>> m_models;
  CountryParentNameGetterFn m_countryParentNameGetterFn;
};

HighwayBasedFactors GetOneFactorsForBicycleAndPedestrianModel();

std::string DebugPrint(VehicleModelInterface::RoadAvailability const l);
std::string DebugPrint(SpeedKMpH const & speed);
std::string DebugPrint(SpeedFactor const & speedFactor);
std::string DebugPrint(InOutCitySpeedKMpH const & speed);
std::string DebugPrint(InOutCityFactor const & speedFactor);
std::string DebugPrint(HighwayType type);
}  // namespace routing
