#pragma once

#include <array>
#include <functional>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Classificator;
class FeatureType;

namespace feature { class TypesHolder; }

namespace routing
{

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

  /// @return Allowed speed in KMpH.
  /// 0 means that it's forbidden to move on this feature or it's not a road at all.
  virtual double GetSpeed(FeatureType const & f) const = 0;

  /// @returns Max speed in KMpH for this model
  virtual double GetMaxSpeed() const = 0;

  /// @return Offroad speed in KMpH for vehicle. This speed should be used for non-feature routing
  /// e.g. to connect start point to nearest feature.
  virtual double GetOffroadSpeed() const = 0;

  virtual bool IsOneWay(FeatureType const & f) const = 0;

  /// @returns true iff feature |f| can be used for routing with corresponding vehicle model.
  virtual bool IsRoad(FeatureType const & f) const = 0;

  /// @returns true iff feature |f| can be used for through passage with corresponding vehicle model.
  /// e.g. in Russia roads tagged "highway = service" are not allowed for through passage;
  /// however, road with this tag can be be used if it is close enough to the start or destination
  /// point of the route.
  /// Roads with additional types e.g. "path = ferry", "vehicle_type = yes" considered as allowed
  /// to pass through.
  virtual bool IsPassThroughAllowed(FeatureType const & f) const = 0;
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
    char const * m_types[2];      // 2-arity road type
    double m_speedKMpH;           // max allowed speed on this road type
    bool m_isPassThroughAllowed;  // pass through this road type is allowed
  };

  // Structure for keeping surface tags: psurface|paved_good, psurface|paved_bad,
  // psurface|unpaved_good and psurface|unpaved_bad.
  struct FeatureTypeSurface
  {
    char const * m_types[2];  // 2-arity road type
    double m_speedFactor;     // Factor (lowering) which reduces speed on feature in case of
                              // bad pavement. It should be from 0.0 to 1.0.
  };

  struct AdditionalRoadTags final
  {
    AdditionalRoadTags() = default;

    AdditionalRoadTags(std::initializer_list<char const *> const & hwtag, double speedKMpH)
      : m_hwtag(hwtag), m_speedKMpH(speedKMpH)
    {
    }

    std::initializer_list<char const *> m_hwtag;
    double m_speedKMpH = 0.0;
  };

  typedef std::initializer_list<FeatureTypeLimits> LimitsInitList;
  typedef std::initializer_list<FeatureTypeSurface> SurfaceInitList;

  VehicleModel(Classificator const & c, LimitsInitList const & featureTypeLimits,
               SurfaceInitList const & featureTypeSurface);

  /// VehicleModelInterface overrides:
  double GetSpeed(FeatureType const & f) const override;
  double GetMaxSpeed() const override { return m_maxSpeedKMpH; }
  bool IsOneWay(FeatureType const & f) const override;
  bool IsRoad(FeatureType const & f) const override;
  bool IsPassThroughAllowed(FeatureType const & f) const override;

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

  double GetMinTypeSpeed(feature::TypesHolder const & types) const;

  double m_maxSpeedKMpH;

private:
  struct AdditionalRoadType final
  {
    AdditionalRoadType(Classificator const & c, AdditionalRoadTags const & tag);

    bool operator==(AdditionalRoadType const & rhs) const { return m_type == rhs.m_type; }
    uint32_t const m_type;
    double const m_speedKMpH;
  };

  class RoadLimits final
  {
  public:
    RoadLimits() = delete;
    RoadLimits(double speedKMpH, bool isPassThroughAllowed);

    double GetSpeedKMpH() const { return m_speedKMpH; };
    bool IsPassThroughAllowed() const { return m_isPassThroughAllowed; };
    bool operator==(RoadLimits const & rhs) const
    {
      return (m_speedKMpH == rhs.m_speedKMpH) &&
             (m_isPassThroughAllowed == rhs.m_isPassThroughAllowed);
    }

  private:
    double const m_speedKMpH;
    bool const m_isPassThroughAllowed;
  };

  struct TypeFactor
  {
    uint32_t m_type = 0;
    double m_factor = 1.0;
  };

  std::vector<AdditionalRoadType>::const_iterator FindRoadType(uint32_t type) const;

  std::unordered_map<uint32_t, RoadLimits> m_highwayTypes;
  // Mapping surface types (psurface|paved_good, psurface|paved_bad, psurface|unpaved_good,
  // psurface|unpaved_bad) to surface speed factors.
  // Note. It's a vector (not map or unordered_map) because of perfomance reasons.
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
}  // namespace routing
