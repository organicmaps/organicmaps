#include "generator/affiliation.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/thread_pool_computational.hpp"

#include <cmath>
#include <functional>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/geometries/register/point.hpp>

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y);
BOOST_GEOMETRY_REGISTER_RING(std::vector<m2::PointD>);

namespace feature
{
CountriesFilesAffiliation::CountriesFilesAffiliation(std::string const & borderPath, bool haveBordersForWholeWorld)
  : m_countryPolygonsTree(borders::GetOrCreateCountryPolygonsTree(borderPath))
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{
}

std::vector<std::string> CountriesFilesAffiliation::GetAffiliations(FeatureBuilder const & fb) const
{
  std::vector<std::string> countries;
  std::vector<std::reference_wrapper<borders::CountryPolygons const>> countriesContainer;
  m_countryPolygonsTree.ForEachCountryInRect(fb.GetLimitRect(), [&](auto const & countryPolygons) {
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

std::vector<std::string>
CountriesFilesAffiliation::GetAffiliations(m2::PointD const & point) const
{
  std::vector<std::string> countries;
  std::vector<std::reference_wrapper<borders::CountryPolygons const>> countriesContainer;
  m_countryPolygonsTree.ForEachCountryInRect(m2::RectD(point, point), [&](auto const & countryPolygons) {
    countriesContainer.emplace_back(countryPolygons);
  });

  if (m_haveBordersForWholeWorld && countriesContainer.size() == 1)
  {
    borders::CountryPolygons const & countryPolygons = countriesContainer.front();
    countries.emplace_back(countryPolygons.GetName());
    return countries;
  }

  for (borders::CountryPolygons const & countryPolygons : countriesContainer)
  {
    auto const need = countryPolygons.Contains(point);

    if (need)
      countries.emplace_back(countryPolygons.GetName());
  }

  return countries;
}

bool CountriesFilesAffiliation::HasCountryByName(std::string const & name) const
{
  return m_countryPolygonsTree.HasRegionByName(name);
}

CountriesFilesIndexAffiliation::CountriesFilesIndexAffiliation(std::string const & borderPath,
                                                               bool haveBordersForWholeWorld)
  : CountriesFilesAffiliation(borderPath, haveBordersForWholeWorld)
{
  static std::mutex cacheMutex;
  static std::unordered_map<std::string, std::shared_ptr<Tree>> cache;
  auto const key = borderPath + std::to_string(haveBordersForWholeWorld);
  {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto const it = cache.find(key);
    if (it != std::cend(cache))
    {
      m_index = it->second;
      return;
    }
  }
  auto const net = generator::cells_merger::MakeNet(0.2 /* step */,
                                                    mercator::Bounds::kMinX, mercator::Bounds::kMinY,
                                                    mercator::Bounds::kMaxX, mercator::Bounds::kMaxY);
  auto const index = BuildIndex(net);
  m_index = index;
  std::lock_guard<std::mutex> lock(cacheMutex);
  cache.emplace(key, index);
}

std::vector<std::string> CountriesFilesIndexAffiliation::GetAffiliations(FeatureBuilder const & fb) const
{
  auto const oneCountry = IsOneCountryForBbox(fb);
  return oneCountry ? std::vector<std::string>{*oneCountry} : GetHonestAffiliations(fb);
}

std::optional<std::string> CountriesFilesIndexAffiliation::IsOneCountryForBbox(
    FeatureBuilder const & fb) const
{
  borders::CountryPolygons const * country = nullptr;
  std::vector<Value> values;
  auto const bbox = MakeBox(fb.GetLimitRect());
  boost::geometry::index::query(*m_index, boost::geometry::index::covers(bbox),
                                std::back_inserter(values));
  for (auto const & v : values)
  {
    for (borders::CountryPolygons const & c : v.second)
    {
      if (!country)
        country = &c;
      else if (country != &c)
        return {};
    }
  }
  return country ? country->GetName() : std::optional<std::string>{};
}

std::vector<std::string> CountriesFilesIndexAffiliation::GetHonestAffiliations(FeatureBuilder const & fb) const
{
  std::vector<std::string> affiliations;
  std::unordered_set<borders::CountryPolygons const *> countires;
  fb.ForEachGeometryPoint([&](auto const & point) {
    std::vector<Value> values;
    boost::geometry::index::query(*m_index, boost::geometry::index::covers(point),
                                  std::back_inserter(values));
    for (auto const & v : values)
    {
      if (v.second.size() == 1)
      {
        borders::CountryPolygons const & cp = v.second.front();
        if (countires.insert(&cp).second)
          affiliations.emplace_back(cp.GetName());
      }
      else
      {
        for (borders::CountryPolygons const & cp : v.second)
        {
          if (cp.Contains(point) && countires.insert(&cp).second)
            affiliations.emplace_back(cp.GetName());
        }
      }
    }
    return true;
  });
  return affiliations;
}

// static
CountriesFilesIndexAffiliation::Box CountriesFilesIndexAffiliation::MakeBox(m2::RectD const & rect)
{
  return {rect.LeftBottom(), rect.RightTop()};
}

std::shared_ptr<CountriesFilesIndexAffiliation::Tree>
CountriesFilesIndexAffiliation::BuildIndex(const std::vector<m2::RectD> & net)
{
  std::unordered_map<borders::CountryPolygons const *, std::vector<m2::RectD>> countriesRects;
  std::mutex countriesRectsMutex;
  std::vector<Value> treeCells;
  std::mutex treeCellsMutex;
  auto const numThreads = GetPlatform().CpuCores();
  {
    base::thread_pool::computational::ThreadPool pool(numThreads);
    for (auto const & rect : net)
    {
      pool.SubmitWork([&, rect]() {
        std::vector<std::reference_wrapper<borders::CountryPolygons const>> countries;
        m_countryPolygonsTree.ForEachCountryInRect(rect, [&](auto const & country) {
          countries.emplace_back(country);
        });
        if (m_haveBordersForWholeWorld && countries.size() == 1)
        {
          borders::CountryPolygons const & country = countries.front();
          std::lock_guard<std::mutex> lock(countriesRectsMutex);
          countriesRects[&country].emplace_back(rect);
        }
        else
        {
          auto const box = MakeBox(rect);
          std::vector<std::reference_wrapper<borders::CountryPolygons const>> interCountries;
          for (borders::CountryPolygons const & cp : countries)
          {
            cp.ForAnyPolygon([&](auto const & polygon) {
              if (!boost::geometry::intersects(polygon.Data(), box))
                return false;
              interCountries.emplace_back(cp);
              return true;
            });
          }
          if (interCountries.empty())
            return;
          if (interCountries.size() == 1)
          {
            borders::CountryPolygons const & country = interCountries.front();
            std::lock_guard<std::mutex> lock(countriesRectsMutex);
            countriesRects[&country].emplace_back(rect);
          }
          else
          {
            std::lock_guard<std::mutex> lock(treeCellsMutex);
            treeCells.emplace_back(box, std::move(interCountries));
          }
        }
      });
    }
  }
  {
    base::thread_pool::computational::ThreadPool pool(numThreads);
    for (auto & pair : countriesRects)
    {
      pool.SubmitWork([&, countryPtr{pair.first}, rects{std::move(pair.second)}]() mutable {
        generator::cells_merger::CellsMerger merger(std::move(rects));
        auto const merged = merger.Merge();
        for (auto const & rect : merged)
        {
          std::vector<std::reference_wrapper<borders::CountryPolygons const>> interCountries{*countryPtr};
          std::lock_guard<std::mutex> lock(treeCellsMutex);
          treeCells.emplace_back(MakeBox(rect), std::move(interCountries));
        }
      });
    }
  }
  return std::make_shared<Tree>(treeCells);
}

SingleAffiliation::SingleAffiliation(std::string const & filename)
  : m_filename(filename)
{
}

std::vector<std::string> SingleAffiliation::GetAffiliations(FeatureBuilder const &) const
{
  return {m_filename};
}

bool SingleAffiliation::HasCountryByName(std::string const & name) const
{
  return name == m_filename;
}

std::vector<std::string>
SingleAffiliation::GetAffiliations(m2::PointD const & point) const
{
  return {m_filename};
}
}  // namespace feature
