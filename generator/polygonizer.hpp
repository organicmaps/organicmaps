#pragma once

#include "generator/borders.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/mercator.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace feature
{
// Groups features according to country polygons.
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
            ("Error loading country polygons files."));
    }
    else
    {
      // Insert fake country polygon equal to whole world to
      // create only one output file which contains all features.
      borders::RegionsContainer regions;
      auto const rect = MercatorBounds::FullRect();
      std::vector<m2::PointD> points {rect.LeftBottom(), rect.LeftTop(), rect.RightTop(),
            rect.RightBottom(), rect.LeftBottom()};
      regions.Add(m2::RegionD(std::move(points)), rect);
      auto countries = borders::CountryPolygons(info.m_fileName, regions);
      m_countries.Add(std::move(countries), rect);
    }
  }

  ~Polygonizer()
  {
    Finish();
  }

  void operator()(FeatureBuilder & fb)
  {
    m_countries.ForEachInRect(fb.GetLimitRect(), [&](auto const & countryPolygons) {
      auto const need = fb.ForAnyGeometryPoint([&](auto const & point) {
        return countryPolygons.Contains(point);
      });

      if (need)
        this->EmitFeature(countryPolygons, fb);
    });
  }

  void Start()
  {
    m_currentNames.clear();
  }

  void Finish()
  {
  }

  void EmitFeature(borders::CountryPolygons const & countryPolygons, FeatureBuilder const & fb)
  {
    if (countryPolygons.m_index == -1)
    {
      m_names.push_back(countryPolygons.GetName());
      m_buckets.emplace_back(new FeatureOut(m_info.GetTmpFileName(countryPolygons.GetName())));
      countryPolygons.m_index = static_cast<int>(m_buckets.size()) - 1;
    }

    if (!m_currentNames.empty())
      m_currentNames += ';';

    m_currentNames += countryPolygons.GetName();
    m_buckets[countryPolygons.m_index]->Collect(fb);
  }

  std::vector<std::string> const & GetNames() const
  {
    return m_names;
  }

  std::string const & GetCurrentNames() const
  {
    return m_currentNames;
  }

private:
  feature::GenerateInfo const & m_info;
  std::vector<std::unique_ptr<FeatureOut>> m_buckets;
  std::vector<std::string> m_names;
  borders::CountriesContainer m_countries;
  std::string m_currentNames;
};
}  // namespace feature
