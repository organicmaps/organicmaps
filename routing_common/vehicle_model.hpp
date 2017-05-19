#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Classificator;
class FeatureType;

namespace feature { class TypesHolder; }

namespace routing
{

class IVehicleModel
{
public:
  enum class RoadAvailability
  {
    NotAvailable,
    Available,
    Unknown,
  };

  virtual ~IVehicleModel() {}

  /// @return Allowed speed in KMpH.
  /// 0 means that it's forbidden to move on this feature or it's not a road at all.
  virtual double GetSpeed(FeatureType const & f) const = 0;

  /// @returns Max speed in KMpH for this model
  virtual double GetMaxSpeed() const = 0;

  virtual bool IsOneWay(FeatureType const & f) const = 0;

  /// @returns true iff feature |f| can be used for routing with corresponding vehicle model.
  virtual bool IsRoad(FeatureType const & f) const = 0;

  /// @returns true iff feature |f| can be used for transit with corresponding vehicle model.
  virtual bool IsTransitAllowed(FeatureType const & f) const = 0;
};

class VehicleModelFactory
{
public:
  virtual ~VehicleModelFactory() {}
  /// @return Default vehicle model which corresponds for all countrines,
  /// but it may be non optimal for some countries
  virtual std::shared_ptr<IVehicleModel> GetVehicleModel() const = 0;

  /// @return The most optimal vehicle model for specified country
  virtual std::shared_ptr<IVehicleModel> GetVehicleModelForCountry(
      std::string const & country) const = 0;
};

class VehicleModel : public IVehicleModel
{
public:
  struct FeatureTypeLimits final
  {
    char const * m_types[2];  /// 2-arity road type
    double m_speedKMpH;       /// max allowed speed on this road type
    bool m_isTransitAllowed;  /// transit allowed for this road type
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

  typedef std::initializer_list<FeatureTypeLimits> InitListT;

  VehicleModel(Classificator const & c, InitListT const & featureTypeLimits);

  /// IVehicleModel overrides:
  double GetSpeed(FeatureType const & f) const override;
  double GetMaxSpeed() const override { return m_maxSpeedKMpH; }
  bool IsOneWay(FeatureType const & f) const override;
  bool IsRoad(FeatureType const & f) const override;
  bool IsTransitAllowed(FeatureType const & f) const override;

public:
  /// @returns true if |m_types| or |m_addRoadTypes| contains |type| and false otherwise.
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

  bool HasTransitType(feature::TypesHolder const & types) const;

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
    RoadLimits(double speedKMpH, bool isTransitAllowed);

    double GetSpeedKMpH() const { return m_speedKMpH; };
    bool IsTransitAllowed() const { return m_isTransitAllowed; };

  private:
    double const m_speedKMpH;
    bool const m_isTransitAllowed;
  };

  std::vector<AdditionalRoadType>::const_iterator FindRoadType(uint32_t type) const;

  std::unordered_map<uint32_t, RoadLimits> m_types;

  std::vector<AdditionalRoadType> m_addRoadTypes;
  uint32_t m_onewayType;
};

std::string DebugPrint(IVehicleModel::RoadAvailability const l);
}  // namespace routing
