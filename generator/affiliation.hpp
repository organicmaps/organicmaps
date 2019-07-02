#pragma once

#include "generator/borders.hpp"
#include "generator/feature_builder.hpp"

#include <string>
#include <vector>

namespace feature
{
class AffiliationInterface
{
public:
  virtual ~AffiliationInterface() = default;

  virtual std::vector<std::string> GetAffiliations(FeatureBuilder const & fb) const = 0;
  virtual bool HasRegionByName(std::string const & name) const = 0;
};

class CountriesFilesAffiliation : public AffiliationInterface
{
public:
  CountriesFilesAffiliation(std::string const & borderPath, bool isMwmsForWholeWorld);

  // AffiliationInterface overrides:
  std::vector<std::string> GetAffiliations(FeatureBuilder const & fb) const override;

  bool HasRegionByName(std::string const & name) const override;

private:
  borders::CountriesContainer const & m_countries;
  bool m_isMwmsForWholeWorld;
};

class OneFileAffiliation : public AffiliationInterface
{
public:
  OneFileAffiliation(std::string const & filename);

  // AffiliationInterface overrides:
  std::vector<std::string> GetAffiliations(FeatureBuilder const &) const override;

  bool HasRegionByName(std::string const & name) const override;

private:
  std::string m_filename;
};
}  // namespace feature
