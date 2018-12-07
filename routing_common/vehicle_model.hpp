#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include <array>
#include <functional>
#include <initializer_list>
#include <memory>
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

class VehicleModelInterface
{
public:
  enum class RoadAvailability
  {
    NotAvailable,
    Available,
    Unknown,
  };

  /// Speeds which are used for edge weight and ETA estimations.
  struct SpeedKMpH
  {
    constexpr SpeedKMpH() = default;
    constexpr explicit SpeedKMpH(double weight) noexcept : m_weight(weight), m_eta(weight) {}
    constexpr SpeedKMpH(double weight, double eta) noexcept : m_weight(weight), m_eta(eta) {}

    bool operator==(SpeedKMpH const & rhs) const
    {
      return m_weight == rhs.m_weight && m_eta == rhs.m_eta;
    }

    bool IsValid() const { return m_weight >= 0 && m_eta >= 0; }

    double m_weight = 0.0; // KMpH
    double m_eta = 0.0;    // KMpH
  };

  /// \brief Factors which reduce weight and ETA speed on feature regarding maxspeed tag value
  /// if the tag is set for the feature.
  /// Both should be in range [0.0, 1.0].
  struct MaxspeedFactor
  {
    constexpr explicit MaxspeedFactor(double weight) noexcept : m_weight(weight), m_eta(weight) {}
    constexpr explicit MaxspeedFactor(double weight, double eta) noexcept : m_weight(weight), m_eta(eta) {}

    bool operator==(MaxspeedFactor const & rhs) const { return m_weight == rhs.m_weight && m_eta == rhs.m_eta; }

    double m_weight;
    double m_eta;
  };

  struct InOutCitySpeedKMpH
  {
    constexpr InOutCitySpeedKMpH(SpeedKMpH const & inCity, SpeedKMpH const & outCity,
                                 MaxspeedFactor const & maxspeedFactor) noexcept
      : m_inCity(inCity), m_outCity(outCity), m_maxspeedFactor(maxspeedFactor)
    {
    }

    bool operator==(InOutCitySpeedKMpH const & rhs) const
    {
      return m_inCity == rhs.m_inCity && m_outCity == rhs.m_outCity && m_maxspeedFactor == rhs.m_maxspeedFactor;
    }

    SpeedKMpH m_inCity;
    SpeedKMpH m_outCity;
    // If maxspeed is available it should be multiplied by |m_maxspeedFactor|.
    MaxspeedFactor m_maxspeedFactor;
  };

  /// Factors which reduce weight and ETA speed on feature in case of bad pavement.
  /// Both should be in range [0.0, 1.0].
  struct SpeedFactor
  {
    double m_weight = 1.0;
    double m_eta = 1.0;
  };

  virtual ~VehicleModelInterface() = default;

  /// @return Allowed weight and ETA speed in KMpH.
  /// 0 means that it's forbidden to move on this feature or it's not a road at all.
  /// @param inCity is true if |f| lies in a city of town.
  virtual SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const = 0;

  virtual double GetMaxWeightSpeed() const = 0;

  /// @return Offroad speed in KMpH for vehicle. This speed should be used for non-feature routing
  /// e.g. to connect start point to nearest feature.
  virtual double GetOffroadSpeed() const = 0;

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
  using InOutCitySpeedKMpH = VehicleModelInterface::InOutCitySpeedKMpH;
  using SpeedFactor = VehicleModelInterface::SpeedFactor;
  using SpeedKMpH = VehicleModelInterface::SpeedKMpH;

  struct FeatureTypeLimits final
  {
    FeatureTypeLimits(std::vector<std::string> const & types, InOutCitySpeedKMpH const & speed,
                      bool isPassThroughAllowed)
      : m_types(types)
      , m_speed(speed)
      , m_isPassThroughAllowed(isPassThroughAllowed)
    {
    }

    std::vector<std::string> m_types;
    InOutCitySpeedKMpH m_speed;   // Average speed on this road type in and out of cities.
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
               SurfaceInitList const & featureTypeSurface);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const override;
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
    return (m_highwayTypes == rhs.m_highwayTypes) && (m_addRoadTypes == rhs.m_addRoadTypes) &&
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

  /// \brief maximum within all the speed limits set in a model (car model, bicycle modle and so on).
  /// It shouldn't be mixed with maxspeed value tag which defines maximum legal speed on a feature.
  InOutCitySpeedKMpH m_modelMaxSpeed;

private:
  struct AdditionalRoadType final
  {
    AdditionalRoadType(Classificator const & c, AdditionalRoadTags const & tag);

    bool operator==(AdditionalRoadType const & rhs) const { return m_type == rhs.m_type; }

    uint32_t const m_type;
    InOutCitySpeedKMpH m_speed;
  };

  class RoadLimits final
  {
  public:
    RoadLimits(InOutCitySpeedKMpH const & speed, bool isPassThroughAllowed);

    SpeedKMpH const & GetSpeed(bool inCity) const { return inCity ? m_speed.m_inCity : m_speed.m_outCity; };
    MaxspeedFactor const & GetMaxspeedFactor() const { return m_speed.m_maxspeedFactor; }
    bool IsPassThroughAllowed() const { return m_isPassThroughAllowed; };
    bool operator==(RoadLimits const & rhs) const
    {
      return (m_speed == rhs.m_speed) && (m_isPassThroughAllowed == rhs.m_isPassThroughAllowed);
    }

  private:
    InOutCitySpeedKMpH const m_speed;
    bool const m_isPassThroughAllowed;
  };

  struct TypeFactor
  {
    uint32_t m_type = 0;
    SpeedFactor m_factor;
  };

  std::vector<AdditionalRoadType>::const_iterator FindRoadType(uint32_t type) const;

  std::unordered_map<uint32_t, RoadLimits> m_highwayTypes;
  // Mapping surface types (psurface|paved_good, psurface|paved_bad, psurface|unpaved_good,
  // psurface|unpaved_bad) to surface speed factors.
  // Note. It's an array (not map or unordered_map) because of perfomance reasons.
  std::array<TypeFactor, 4> m_surfaceFactors;

  std::vector<AdditionalRoadType> m_addRoadTypes;
  uint32_t m_onewayType;
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

std::string DebugPrint(VehicleModelInterface::RoadAvailability const l);
std::string DebugPrint(VehicleModelInterface::SpeedKMpH const & speed);
std::string DebugPrint(VehicleModelInterface::InOutCitySpeedKMpH const & speed);
}  // namespace routing
