#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "base/small_map.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace feature { class TypesHolder; }
class FeatureType;

namespace routing
{
double constexpr kNotUsed = std::numeric_limits<double>::max();

// Each value is equal to the corresponding type index from types.txt.
// Check for static_cast<HighwayType> in vehicle_model.cpp
enum class HighwayType : uint16_t
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
  RailwayRailMotorVehicle = 994,
  RouteShuttleTrain = 1054,
};

// Each value is equal to the corresponding type index from types.txt.
// Check for static_cast<SurfaceType> in vehicle_model.cpp
enum class SurfaceType : uint16_t
{
  PavedGood = 1116,
  PavedBad = 1117,
  UnpavedGood = 1118,
  UnpavedBad = 1119,
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

/// \brief Factors which modify weight and ETA speed on feature in case of
/// bad pavement (reduce) or good highway class (increase).
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
  constexpr InOutCityFactor() = default;
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

using HighwaySpeeds = std::unordered_map<HighwayType, InOutCitySpeedKMpH>;
using HighwayFactors = std::unordered_map<HighwayType, InOutCityFactor>;
using SurfaceFactors = std::unordered_map<SurfaceType, SpeedFactor>;
using NoPassThroughHighways = std::unordered_set<HighwayType>;


class VehicleModelInterface
{
public:
  virtual ~VehicleModelInterface() = default;

  struct Request
  {
    Maxspeed m_speed;
    bool m_inCity;
  };
  struct Response
  {
    SpeedKMpH m_forwardSpeed, m_backwardSpeed;

    // Highway type can be null, but m_isValid == true if Feature has hwtype=yes*
    // without any recognizable highway classifier type in it.
    std::optional<HighwayType> m_highwayType;
    bool m_isValid = false;

    bool m_isOneWay;
    bool m_isPassThroughAllowed;
  };
  virtual Response GetFeatureInfo(FeatureType & ft, Request const & request) const = 0;

  /// @todo Remove after proper FeaturesRoadGraph refactoring.
  virtual bool HasRoadType(FeatureType & ft) const = 0;
  virtual bool IsOneWay(FeatureType & ft) const = 0;

  /// @return Maximum model weight speed.
  /// All speeds which the model returns must be less than or equal to this speed.
  virtual double GetMaxWeightSpeed() const = 0;

  /// @return Offroad speed in KMpH for vehicle. This speed should be used for non-feature routing
  /// e.g. to connect start point to nearest feature.
  virtual SpeedKMpH GetOffroadSpeed() const = 0;
};

class VehicleModelFactoryInterface
{
public:
  virtual ~VehicleModelFactoryInterface() = default;
  /// @return Default vehicle model which corresponds for all countrines,
  /// but it may be non optimal for some countries
  virtual std::shared_ptr<VehicleModelInterface> GetVehicleModel() const = 0;

  /// @return The most optimal vehicle model for specified \a country
  virtual std::shared_ptr<VehicleModelInterface> GetVehicleModelForCountry(
      std::string const & country) const = 0;
};

class VehicleModel : public VehicleModelInterface
{
public:
  VehicleModel(HighwaySpeeds const & speeds, HighwayFactors const & factors,
               SurfaceFactors const & surfaces, NoPassThroughHighways const & noPassThrough = {});

  /// @name VehicleModelInterface overrides.
  /// @{
  Response GetFeatureInfo(FeatureType & ft, Request const & request) const override;

  bool HasRoadType(FeatureType & ft) const override;
  bool IsOneWay(FeatureType & ft) const override;

  double GetMaxWeightSpeed() const override;
  /// @}

  Response GetFeatureInfo(feature::TypesHolder const & types, Request const & request) const;

  template <class ContT> bool HasRoadType(ContT const & types) const
  {
    for (uint32_t type : types)
    {
      NormalizeType(type);
      if (m_highwayInfo.Find(type))
        return true;
    }
    return false;
  }

  bool EqualsForTests(VehicleModel const & rhs) const
  {
    return (m_highwayInfo == rhs.m_highwayInfo && m_highwayInfo == rhs.m_highwayInfo &&
            m_maxModelSpeed == rhs.m_maxModelSpeed);
  }

protected:
  enum class ResultT
  {
    Yes, No, Unknown
  };

  /// @returns If oneway road.
  virtual ResultT IsOneWay(uint32_t type) const = 0;

  /// @returns Availability according to a special restriction: hwtag={yes,no}{car,bicycle,foot}.
  virtual ResultT GetRoadAvailability(uint32_t type) const = 0;

  /// @returns Default speed for available road, but not in the current HighwayType model.
  virtual SpeedKMpH GetSpeedForAvailable() const = 0;

  /// \brief Maximum within all the speed limits set in a model (car model, bicycle model and so on).
  /// Do not mix with maxspeed value tag, which defines maximum legal speed on a feature.
  InOutCitySpeedKMpH m_maxModelSpeed;

private:
  void NormalizeType(uint32_t type) const;

  struct Info
  {
    InOutCitySpeedKMpH m_speed;
    InOutCityFactor m_factor;
    bool m_isPassThroughAllowed = true;

    bool operator==(Info const & rhs) const
    {
      return (m_speed == rhs.m_speed && m_factor == rhs.m_factor &&
              m_isPassThroughAllowed == rhs.m_isPassThroughAllowed);
    }
  };

  SpeedKMpH GetSpeed(Info const * info, MaxspeedType ftMaxSpeed, bool inCity) const;

  base::SmallMap<uint32_t, Info> m_highwayInfo;

  base::SmallMapBase<uint32_t, SpeedFactor> m_surfaceFactors;
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

HighwayFactors GetOneFactorsForBicycleAndPedestrianModel();

std::string DebugPrint(SpeedKMpH const & speed);
std::string DebugPrint(SpeedFactor const & speedFactor);
std::string DebugPrint(InOutCitySpeedKMpH const & speed);
std::string DebugPrint(InOutCityFactor const & speedFactor);
std::string DebugPrint(HighwayType type);

}  // namespace routing
