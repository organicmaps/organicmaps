#include "generator/isolines_generator.hpp"

#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

#include "indexer/classificator.hpp"

namespace generator
{
namespace
{
std::vector<int> const kAltClasses = {1000, 500, 100, 50, 10};
int const kNamedAltStep = 50;
std::string const kTypePrefix = "step_";
std::string const kTypeZero = "zero";
uint32_t const kInvalidType = 0;

std::string GetIsolineName(topography_generator::Altitude altitude)
{
  if (altitude % kNamedAltStep == 0)
    return strings::to_string(altitude);
  return "";
}
}  // namespace

IsolineFeaturesGenerator::IsolineFeaturesGenerator(std::string const & isolinesDir)
  : m_isolinesDir(isolinesDir)
{
  ASSERT(std::is_sorted(kAltClasses.cbegin(), kAltClasses.cend(), std::greater<int>()), ());
  for (auto alt : kAltClasses)
  {
    auto const type = kTypePrefix + strings::to_string(alt);
    m_altClassToType[alt] = classif().GetTypeByPath({"isoline", type});
  }
  m_altClassToType[0] = classif().GetTypeByPath({"isoline", kTypeZero});
}

uint32_t IsolineFeaturesGenerator::GetIsolineType(int altitude) const
{
  if (altitude == 0)
    return m_altClassToType.at(0);

  for (auto altStep : kAltClasses)
  {
    if (altitude % altStep == 0)
      return m_altClassToType.at(altStep);
  }
  return kInvalidType;
}

void IsolineFeaturesGenerator::GenerateIsolines(std::string const & countryName,
                                                FeaturesCollectFn const & fn) const
{
  auto const isolinesPath = topography_generator::GetIsolinesFilePath(countryName,
                                                                      m_isolinesDir);
  topography_generator::Contours<topography_generator::Altitude> countryIsolines;
  if (!topography_generator::LoadContours(isolinesPath, countryIsolines))
    return;

  for (auto const & levelIsolines : countryIsolines.m_contours)
  {
    auto const altitude = levelIsolines.first;
    auto const isolineName = GetIsolineName(altitude);
    auto const isolineType = GetIsolineType(altitude);
    if (isolineType == kInvalidType)
    {
      LOG(LWARNING, ("Skip unsupported altitudes level", altitude, "in", countryName));
      continue;
    }

    for (auto const & isoline : levelIsolines.second)
    {
      feature::FeatureBuilder fb;
      for (auto const & pt : isoline)
        fb.AddPoint(pt);
      fb.AddType(isolineType);
      if (!isolineName.empty())
        fb.AddName("default", isolineName);
      fb.SetLinear();
      fn(std::move(fb));
    }
  }
}
}  // namespace generator
