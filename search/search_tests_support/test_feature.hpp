#pragma once

#include "geometry/point2d.hpp"

#include "std/string.hpp"

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

  virtual void Serialize(FeatureBuilder1 & fb) const;
  virtual bool Matches(FeatureType const & feature) const;
  virtual string ToString() const = 0;

protected:
  TestFeature(m2::PointD const & center, string const & name, string const & lang);

  m2::PointD const m_center;
  string const m_name;
  string const m_lang;
};

class TestPOI : public TestFeature
{
public:
  TestPOI(m2::PointD const & center, string const & name, string const & lang);

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

string DebugPrint(TestFeature const & feature);
}  // namespace tests_support
}  // namespace search
