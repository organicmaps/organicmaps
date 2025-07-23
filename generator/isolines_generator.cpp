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
int const kNamedAltRange = 150;
std::string_view const kIsoline = "isoline";
std::string const kTypePrefix = "step_";
std::string_view const kTypeZero = "zero";

std::string GetIsolineName(int altitude, int step, int minAltitude, int maxAltitude)
{
  if (step > 10 || abs(altitude) % kNamedAltStep == 0 || maxAltitude - minAltitude <= kNamedAltRange)
    return strings::to_string(altitude);
  return "";
}
}  // namespace

IsolineFeaturesGenerator::IsolineFeaturesGenerator(std::string const & isolinesDir) : m_isolinesDir(isolinesDir)
{
  ASSERT(std::is_sorted(kAltClasses.cbegin(), kAltClasses.cend(), std::greater<int>()), ());
  Classificator const & c = classif();

  for (auto alt : kAltClasses)
  {
    auto const type = kTypePrefix + strings::to_string(alt);
    m_altClassToType[alt] = c.GetTypeByPath({kIsoline, type});
  }
  m_altClassToType[0] = c.GetTypeByPath({kIsoline, kTypeZero});
}

uint32_t IsolineFeaturesGenerator::GetIsolineType(int altitude) const
{
  if (altitude == 0)
    return m_altClassToType.at(0);

  for (auto altStep : kAltClasses)
    if (abs(altitude) % altStep == 0)
      return m_altClassToType.at(altStep);
  return ftype::GetEmptyValue();
}

void IsolineFeaturesGenerator::GenerateIsolines(std::string const & countryName, FeaturesCollectFn const & fn) const
{
  auto const isolinesPath = topography_generator::GetIsolinesFilePath(countryName, m_isolinesDir);
  topography_generator::Contours<topography_generator::Altitude> countryIsolines;
  if (!topography_generator::LoadContours(isolinesPath, countryIsolines))
  {
    LOG(LWARNING, ("Can't load contours", isolinesPath));
    return;
  }
  LOG(LINFO, ("Generating isolines for", countryName));
  for (auto & levelIsolines : countryIsolines.m_contours)
  {
    auto const altitude = levelIsolines.first;
    auto const isolineName =
        GetIsolineName(altitude, countryIsolines.m_valueStep, countryIsolines.m_minValue, countryIsolines.m_maxValue);
    auto const isolineType = GetIsolineType(altitude);
    if (isolineType == ftype::GetEmptyValue())
    {
      LOG(LWARNING, ("Skip unsupported altitudes level", altitude, "in", countryName));
      continue;
    }

    for (auto & isoline : levelIsolines.second)
    {
      feature::FeatureBuilder fb;
      fb.AssignPoints(std::move(isoline));

      fb.AddType(isolineType);
      if (!isolineName.empty())
        fb.SetName(StringUtf8Multilang::kDefaultCode, isolineName);

      fb.SetLinear();
      fn(std::move(fb));
    }
  }
}
}  // namespace generator
