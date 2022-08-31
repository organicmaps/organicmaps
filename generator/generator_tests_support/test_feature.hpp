#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

class FeatureType;
namespace feature { class FeatureBuilder; }
namespace osm { class Editor; }

namespace generator
{
namespace tests_support
{
class TestFeature
{
public:
  virtual ~TestFeature() = default;

  bool Matches(FeatureType & feature) const;
  void SetPostcode(std::string const & postcode) { m_postcode = postcode; }
  uint64_t GetId() const { return m_id; }
//  StringUtf8Multilang const & GetNames() const { return m_names; }
  std::string_view GetName(std::string_view lang) const
  {
    std::string_view res;
    if (m_names.GetString(lang, res))
      return res;
    return {};
  }

  m2::PointD const & GetCenter() const { return m_center; }

  feature::Metadata & GetMetadata() { return m_metadata; }

  virtual void Serialize(feature::FeatureBuilder & fb) const;
  virtual std::string ToDebugString() const = 0;

protected:
  enum class Type
  {
    Point,
    Area,
    Unknown
  };

  TestFeature();
  TestFeature(std::string const & name, std::string const & lang);
  TestFeature(StringUtf8Multilang const & name);
  TestFeature(m2::PointD const & center, std::string const & name, std::string const & lang);
  TestFeature(m2::PointD const & center, StringUtf8Multilang const & name);
  TestFeature(std::vector<m2::PointD> const & boundary, std::string const & name,
              std::string const & lang);

  uint64_t const m_id;
  m2::PointD const m_center;
  std::vector<m2::PointD> const m_boundary;
  Type const m_type;
  StringUtf8Multilang m_names;
  std::string m_postcode;
  feature::Metadata m_metadata;

private:
  void Init();
};

class TestCountry : public TestFeature
{
public:
  TestCountry(m2::PointD const & center, std::string const & name, std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;
};

class TestState : public TestFeature
{
public:
  TestState(m2::PointD const & center, std::string const & name, std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;
};

class TestCity : public TestFeature
{
public:
  TestCity(m2::PointD const & center, std::string const & name, std::string const & lang,
           uint8_t rank);
  TestCity(m2::PointD const & center, StringUtf8Multilang const & name, uint8_t rank);
  TestCity(std::vector<m2::PointD> const & boundary, std::string const & name,
           std::string const & lang, uint8_t rank);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  uint8_t const m_rank;
};

class TestVillage : public TestFeature
{
public:
  TestVillage(m2::PointD const & center, std::string const & name, std::string const & lang, uint8_t rank);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  uint8_t const m_rank;
};

class TestStreet : public TestFeature
{
public:
  TestStreet(std::vector<m2::PointD> const & points, std::string const & name, std::string const & lang);
  TestStreet(std::vector<m2::PointD> const & points, StringUtf8Multilang const & name);

  void SetType(base::StringIL const & e);
  void SetRoadNumber(std::string const & roadNumber) { m_roadNumber = roadNumber; }

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  std::vector<m2::PointD> m_points;
  uint32_t m_highwayType;
  std::string m_roadNumber;
};

class TestSquare : public TestFeature
{
public:
  TestSquare(m2::RectD const & rect, std::string const & name, std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  m2::RectD m_rect;
};

class TestPOI : public TestFeature
{
public:
  TestPOI(m2::PointD const & center, std::string const & name, std::string const & lang);

  static std::pair<TestPOI, FeatureID> AddWithEditor(osm::Editor & editor,
                                                     MwmSet::MwmId const & mwmId,
                                                     std::string const & enName,
                                                     m2::PointD const & pt);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

  feature::TypesHolder GetTypes() const;
  void SetHouseNumber(std::string const & houseNumber) { m_houseNumber = houseNumber; }
  void SetStreetName(std::string_view name) { m_streetName = name; }

  void SetType(uint32_t type) { m_types.assign(1 /* count */, type); }
  void SetTypes(std::initializer_list<base::StringIL> const & types);

protected:
  std::string m_houseNumber;
  std::string m_streetName;
  std::vector<uint32_t> m_types;
};

class TestMultilingualPOI : public TestPOI
{
public:
  TestMultilingualPOI(m2::PointD const & center, std::string const & defaultName,
                      std::map<std::string, std::string> const & multilingualNames);
  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  std::map<std::string, std::string> m_multilingualNames;
};

class TestBuilding : public TestFeature
{
public:
  TestBuilding(m2::PointD const & center, std::string const & name, std::string const & houseNumber,
               std::string const & lang);
  TestBuilding(m2::PointD const & center, std::string const & name, std::string const & houseNumber,
               std::string_view street, std::string const & lang);
  TestBuilding(std::vector<m2::PointD> const & boundary, std::string const & name,
               std::string const & houseNumber, std::string_view street,
               std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

  void AddType(std::vector<std::string> const & path) { m_types.push_back(path); }

private:
  std::string const m_houseNumber;
  std::string const m_streetName;

  std::vector<std::vector<std::string>> m_types;
};

class TestPark : public TestFeature
{
public:
  TestPark(std::vector<m2::PointD> const & boundary, std::string const & name, std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;
};

class TestRoad : public TestFeature
{
public:
  TestRoad(std::vector<m2::PointD> const & points, std::string const & name, std::string const & lang);

  // TestFeature overrides:
  void Serialize(feature::FeatureBuilder & fb) const override;
  std::string ToDebugString() const override;

private:
  std::vector<m2::PointD> m_points;
};

std::string DebugPrint(TestFeature const & feature);
}  // namespace tests_support
}  // namespace generator
