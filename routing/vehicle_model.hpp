#pragma once
#include "base/buffer_vector.hpp"

#include "std/initializer_list.hpp"
#include "std/stdint.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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
  typedef initializer_list<SpeedForType> InitListT;

  VehicleModel(Classificator const & c, InitListT const & speedLimits);

  virtual double GetSpeed(FeatureType const & f) const;
  virtual double GetMaxSpeed() const { return m_maxSpeed; }
  virtual bool IsOneWay(FeatureType const & f) const;

  double GetSpeed(feature::TypesHolder const & types) const;
  bool IsOneWay(feature::TypesHolder const & types) const;

  bool IsRoad(FeatureType const & f) const;
  template <class TList> bool IsRoad(TList const & types) const
  {
    for (uint32_t t : types)
      if (IsRoad(t))
        return true;
    return false;
  }
  bool IsRoad(uint32_t type) const;

protected:
  template <size_t N>
  void SetAdditionalRoadTypes(Classificator const & c, initializer_list<char const *> (&arr)[N]);

  double m_maxSpeed;

private:
  typedef unordered_map<uint32_t, double> TypesT;
  TypesT m_types;

  buffer_vector<uint32_t, 4> m_addRoadTypes;
  uint32_t m_onewayType;
};

class CarModel : public VehicleModel
{
public:
  CarModel();
};

class PedestrianModel : public VehicleModel
{
  uint32_t m_noFootType;

  bool IsFoot(feature::TypesHolder const & types) const;

public:
  PedestrianModel();

  virtual double GetSpeed(FeatureType const & f) const;
  virtual bool IsOneWay(FeatureType const &) const { return false; }
};

}
