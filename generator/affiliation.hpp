#pragma once

#include "generator/borders.hpp"
#include "generator/feature_builder.hpp"

#include <string>
#include <vector>

#include <boost/geometry/index/rtree.hpp>

namespace feature
{
class AffiliationInterface
{
public:
  virtual ~AffiliationInterface() = default;

  // The method will return the names of the buckets to which the fb belongs.
  virtual std::vector<std::string> GetAffiliations(FeatureBuilder const & fb) const = 0;
  virtual std::vector<std::string> GetAffiliations(m2::PointD const & point) const = 0;

  virtual bool HasCountryByName(std::string const & name) const = 0;
};

class CountriesFilesAffiliation : public AffiliationInterface
{
public:
  CountriesFilesAffiliation(std::string const & borderPath, bool haveBordersForWholeWorld);

  // AffiliationInterface overrides:
  std::vector<std::string> GetAffiliations(FeatureBuilder const & fb) const override;
  std::vector<std::string> GetAffiliations(m2::PointD const & point) const override;

  std::vector<std::string> GetAffiliations(std::vector<m2::PointD> const & points) const;

  bool HasCountryByName(std::string const & name) const override;

protected:
  borders::CountryPolygonsCollection const & m_countryPolygonsTree;
  bool m_haveBordersForWholeWorld;
};

class CountriesFilesIndexAffiliation : public CountriesFilesAffiliation
{
public:
  using Box = boost::geometry::model::box<m2::PointD>;
  using Value = std::pair<Box, std::vector<std::reference_wrapper<borders::CountryPolygons const>>>;
  using Tree = boost::geometry::index::rtree<Value, boost::geometry::index::quadratic<16>>;

  CountriesFilesIndexAffiliation(std::string const & borderPath, bool haveBordersForWholeWorld);

  // AffiliationInterface overrides:
  std::vector<std::string> GetAffiliations(FeatureBuilder const & fb) const override;
  std::vector<std::string> GetAffiliations(m2::PointD const & point) const override;

private:
  std::shared_ptr<Tree> BuildIndex(std::vector<m2::RectD> const & net);

  std::shared_ptr<Tree> m_index;
};

class SingleAffiliation : public AffiliationInterface
{
public:
  explicit SingleAffiliation(std::string const & filename);

  // AffiliationInterface overrides:
  std::vector<std::string> GetAffiliations(FeatureBuilder const &) const override;
  std::vector<std::string> GetAffiliations(m2::PointD const & point) const override;

  bool HasCountryByName(std::string const & name) const override;

private:
  std::string m_filename;
};
}  // namespace feature

using AffiliationInterfacePtr = std::shared_ptr<feature::AffiliationInterface>;
