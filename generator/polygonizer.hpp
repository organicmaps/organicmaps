#pragma once

#include "generator/borders_loader.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/mercator.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feature
{
// Groups features according to country polygons
template <class FeatureOut>
class Polygonizer
{
public:
  Polygonizer(feature::GenerateInfo const & info)
    : m_info(info)
  {
    if (info.m_splitByPolygons)
    {
      CHECK(borders::LoadCountriesList(info.m_targetDir, m_countries),
            ("Error loading country polygons files"));
    }
    else
    {
      // Insert fake country polygon equal to whole world to
      // create only one output file which contains all features
      m_countries.Add(borders::CountryPolygons(info.m_fileName), MercatorBounds::FullRect());
    }
  }

  ~Polygonizer()
  {
    Finish();
  }

  void operator()(FeatureBuilder1 & fb)
  {
    m_countries.ForEachInRect(fb.GetLimitRect(), [&](auto const & countryPolygons) {
      auto const need = fb.AnyOfGeometryPoint([&](auto const & point) {
        auto const & regions = countryPolygons.m_regions;
        return regions.AnyOfInRect(m2::RectD(point, point), [&](auto const & rgn) {
          return rgn.Contains(point);
        });
        return true;
      });

      if (need)
        EmitFeature(countryPolygons, fb);
    });
  }

  void Start()
  {
    m_currentNames.clear();
  }

  void Finish()
  {
  }

  void EmitFeature(borders::CountryPolygons const & countryPolygons, FeatureBuilder1 const & fb)
  {
    if (countryPolygons.m_index == -1)
    {
      m_Names.push_back(countryPolygons.m_name);
      m_Buckets.emplace_back(new FeatureOut(m_info.GetTmpFileName(countryPolygons.m_name)));
      countryPolygons.m_index = static_cast<int>(m_Buckets.size())-1;
    }

    if (!m_currentNames.empty())
      m_currentNames += ';';

    m_currentNames += countryPolygons.m_name;
    m_Buckets[countryPolygons.m_index]->Collect(fb);
  }

  std::vector<std::string> const & GetNames() const
  {
    return m_Names;
  }

  std::string const & GetCurrentNames() const
  {
    return m_currentNames;
  }

private:
  feature::GenerateInfo const & m_info;
  std::vector<std::unique_ptr<FeatureOut>> m_Buckets;
  std::vector<std::string> m_Names;
  borders::CountriesContainer m_countries;
  std::string m_currentNames;
};
}  // namespace feature
