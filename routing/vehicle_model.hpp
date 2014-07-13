#pragma once
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

class Classificator;
class FeatureType;

namespace feature { class TypesHolder; }

namespace routing
{

class IVehicleModel
{
public:
  virtual ~IVehicleModel() {}

  virtual double GetSpeed(FeatureType const & f) const = 0;
  virtual double GetMaxSpeed() const = 0;
  virtual bool IsOneWay(FeatureType const & f) const = 0;
};

class VehicleModel : public IVehicleModel
{
public:
  struct SpeedForType
  {
    char const * m_types[2];
    double m_speed;
  };

  VehicleModel(Classificator const & c, vector<SpeedForType> const & speedLimits);

  virtual double GetSpeed(FeatureType const & f) const;
  virtual double GetMaxSpeed() const { return m_maxSpeed; }
  virtual bool IsOneWay(FeatureType const & f) const;

  double GetSpeed(feature::TypesHolder const & types) const;
  bool IsOneWay(feature::TypesHolder const & types) const;

private:
  double m_maxSpeed;
  typedef unordered_map<uint32_t, SpeedForType> TypesT;
  TypesT m_types;
  uint32_t m_onewayType;
};

class CarModel : public VehicleModel
{
public:
  CarModel();
};

}
