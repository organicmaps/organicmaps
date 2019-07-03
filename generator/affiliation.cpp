#include "generator/affiliation.hpp"

namespace feature
{
CountriesFilesAffiliation::CountriesFilesAffiliation(std::string const & borderPath, bool haveBordersForWholeWorld)
  : m_countries(borders::PackedBorders::GetOrCreate(borderPath))
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{
}

std::vector<std::string> CountriesFilesAffiliation::GetAffiliations(FeatureBuilder const & fb) const
{
  std::vector<std::string> countries;
  std::vector<std::reference_wrapper<borders::CountryPolygons const>> countriesContainer;
  m_countries.ForEachInRect(fb.GetLimitRect(), [&](auto const & countryPolygons) {
    countriesContainer.emplace_back(countryPolygons);
  });

  // todo(m.andrianov): We need to explore this optimization better. There is a hypothesis: some
  // elements belong to a rectangle, but do not belong to the exact boundary.
  if (m_haveBordersForWholeWorld && countriesContainer.size() == 1)
  {
    borders::CountryPolygons const & countryPolygons = countriesContainer.front();
    countries.emplace_back(countryPolygons.GetName());
    return countries;
  }

  for (borders::CountryPolygons const & countryPolygons : countriesContainer)
  {
    auto const need = fb.ForAnyGeometryPoint([&](auto const & point) {
      return countryPolygons.Contains(point);
    });

    if (need)
      countries.emplace_back(countryPolygons.GetName());
  }

  return countries;
}

bool CountriesFilesAffiliation::HasRegionByName(std::string const & name) const
{
  return m_countries.HasRegionByName(name);
}

SingleAffiliation::SingleAffiliation(std::string const & filename)
  : m_filename(filename)
{
}

std::vector<std::string> SingleAffiliation::GetAffiliations(FeatureBuilder const &) const
{
  return {m_filename};
}

bool SingleAffiliation::HasRegionByName(std::string const & name) const
{
  return name == m_filename;
}
}  // namespace feature
