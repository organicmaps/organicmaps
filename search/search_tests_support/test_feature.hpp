#pragma once

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

class FeatureBuilder1;
class FeatureType;

namespace search
{
namespace tests_support
{
class TestFeature
{
public:
  virtual ~TestFeature() = default;

  bool Matches(FeatureType const & feature) const;
  inline string const & GetName() const { return m_name; }

  virtual void Serialize(FeatureBuilder1 & fb) const;
  virtual string ToString() const = 0;

protected:
  TestFeature(string const & name, string const & lang);
  TestFeature(m2::PointD const & center, string const & name, string const & lang);

  uint64_t const m_id;
  m2::PointD const m_center;
  bool const m_hasCenter;
  string const m_name;
  string const m_lang;
};

class TestCountry : public TestFeature
{
public:
  TestCountry(m2::PointD const & center, string const & name, string const & lang);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;
};

class TestCity : public TestFeature
{
public:
  TestCity(m2::PointD const & center, string const & name, string const & lang, uint8_t rank);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

private:
  uint8_t const m_rank;
};

class TestVillage : public TestFeature
{
public:
  TestVillage(m2::PointD const & center, string const & name, string const & lang, uint8_t rank);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

private:
  uint8_t const m_rank;
};

class TestStreet : public TestFeature
{
public:
  TestStreet(vector<m2::PointD> const & points, string const & name, string const & lang);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

private:
  vector<m2::PointD> m_points;
};

class TestPOI : public TestFeature
{
public:
  TestPOI(m2::PointD const & center, string const & name, string const & lang);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

  inline void SetHouseNumber(string const & houseNumber) { m_houseNumber = houseNumber; }
  inline void SetStreet(TestStreet const & street) { m_streetName = street.GetName(); }
  inline void SetTypes(vector<vector<string>> const & types) { m_types = types; }

private:
  string m_houseNumber;
  string m_streetName;
  vector<vector<string>> m_types;
};

class TestBuilding : public TestFeature
{
public:
  TestBuilding(m2::PointD const & center, string const & name, string const & houseNumber,
               string const & lang);
  TestBuilding(m2::PointD const & center, string const & name, string const & houseNumber,
               TestStreet const & street, string const & lang);
  TestBuilding(vector<m2::PointD> const & boundary, string const & name, string const & houseNumber,
               TestStreet const & street, string const & lang);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

private:
  vector<m2::PointD> const m_boundary;
  string const m_houseNumber;
  string const m_streetName;
};

class TestPark : public TestFeature
{
public:
  TestPark(vector<m2::PointD> const & boundary, string const & name, string const & lang);

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override;
  string ToString() const override;

private:
  vector<m2::PointD> m_boundary;
};

string DebugPrint(TestFeature const & feature);
}  // namespace tests_support
}  // namespace search
