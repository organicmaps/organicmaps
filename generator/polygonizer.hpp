#pragma once

#include "generator/borders_loader.hpp"
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
      auto const rect = MercatorBounds::FullRect();
      auto countries = borders::CountryPolygons(info.m_fileName);
      std::vector<m2::PointD> points {rect.LeftBottom(), rect.LeftTop(), rect.RightTop(),
            rect.RightBottom(), rect.LeftBottom()};
      countries.m_regions.Add(m2::RegionD(std::move(points)), rect);
      m_countries.Add(std::move(countries), rect);
    }
  }

  ~Polygonizer()
  {
    Finish();
  }

  void operator()(FeatureBuilder1 & fb)
  {
    m_countries.ForEachInRect(fb.GetLimitRect(), [&](auto const & countryPolygons) {
      auto const need = fb.ForAnyGeometryPoint([&](auto const & point) {
        auto const & regions = countryPolygons.m_regions;
        return regions.ForAnyInRect(m2::RectD(point, point), [&](auto const & rgn) {
          return rgn.Contains(point);
        });
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

  void EmitFeature(borders::CountryPolygons const & countryPolygons, FeatureBuilder1 const & fb)
  {
    if (countryPolygons.m_index == -1)
    {
      m_names.push_back(countryPolygons.m_name);
      m_buckets.emplace_back(new FeatureOut(m_info.GetTmpFileName(countryPolygons.m_name)));
      countryPolygons.m_index = static_cast<int>(m_buckets.size()) - 1;
    }

    if (!m_currentNames.empty())
      m_currentNames += ';';

    m_currentNames += countryPolygons.m_name;
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
