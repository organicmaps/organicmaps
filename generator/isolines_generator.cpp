#include "generator/isolines_generator.hpp"

#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

#include "indexer/classificator.hpp"

namespace generator
{
namespace
{
std::string GetIsolineType(topography_generator::Altitude altitude)
{
  static std::vector<int> const altClasses = {1000, 500, 100, 50, 10};
  ASSERT(std::is_sorted(altClasses.cbegin(), altClasses.cend(), std::greater<int>()), ());

  std::string const kPrefix = "step_";
  if (altitude == 0)
    return kPrefix + strings::to_string(altClasses.back());

  for (auto altStep : altClasses)
  {
    if (altitude % altStep == 0)
      return kPrefix + strings::to_string(altStep);
  }
  return "";
}
}  // namespace

IsolineFeaturesGenerator::IsolineFeaturesGenerator(std::string const & isolinesDir)
  : m_isolinesDir(isolinesDir)
{}

void IsolineFeaturesGenerator::GenerateIsolines(std::string const & countryName,
                                                std::vector<feature::FeatureBuilder> & fbs) const
{
  auto const isolinesPath = topography_generator::GetIsolinesFilePath(countryName,
                                                                      m_isolinesDir);
  topography_generator::Contours<topography_generator::Altitude> countryIsolines;
  if (!topography_generator::LoadContours(isolinesPath, countryIsolines))
    return;

  for (auto const & levelIsolines : countryIsolines.m_contours)
  {
    auto const altitude = levelIsolines.first;
    auto const isolineName = strings::to_string(altitude);
    auto const isolineType = GetIsolineType(altitude);
    if (isolineType.empty())
    {
      LOG(LWARNING, ("Skip unsupported altitudes level", altitude, "in", countryName));
      continue;
    }
    auto const type = classif().GetTypeByPath({"isoline", isolineType});

    for (auto const & isoline : levelIsolines.second)
    {
      feature::FeatureBuilder fb;
      fb.SetLinear();
      for (auto const & pt : isoline)
        fb.AddPoint(pt);

      fb.AddType(type);
      fb.AddName("default", isolineName);
      fbs.emplace_back(std::move(fb));
    }
  }
}
}  // namespace generator
