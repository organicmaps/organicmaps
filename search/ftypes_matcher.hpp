#pragma once

#include "../base/base.hpp"

#include "../std/vector.hpp"


namespace feature { class TypesHolder; }
class FeatureType;

namespace ftypes
{

class BaseChecker
{
  bool IsMatched(uint32_t t) const;

protected:
  vector<uint32_t> m_types;

public:
  bool operator() (feature::TypesHolder const & types) const;
  bool operator() (FeatureType const & ft) const;
  bool operator() (vector<uint32_t> const & types) const;
};

class IsStreetChecker : public BaseChecker
{
public:
  IsStreetChecker();
};

class IsBuildingChecker : public BaseChecker
{
public:
  IsBuildingChecker();
};

/// Type of locality (do not change values - they have detalization order)
/// COUNTRY < STATE < CITY
enum Type { NONE = -1, COUNTRY = 0, STATE, CITY };

class IsLocalityChecker : public BaseChecker
{
public:
  IsLocalityChecker();

  Type GetLocalityType(feature::TypesHolder const & types) const;
  Type GetLocalityType(FeatureType const & f) const;
};

/// Get city radius (meters)
double GetLocationRadius(FeatureType const & ft);
}
