#include "generator/affiliation.hpp"
#include "generator/cells_merger.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/thread_pool_computational.hpp"

#include <functional>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include "std/boost_geometry.hpp"

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_RING(std::vector<m2::PointD>)

namespace feature
{
namespace affiliation
{
template <typename T>
struct RemoveCvref
{
  typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

template <typename T>
using RemoveCvrefT = typename RemoveCvref<T>::type;

template <typename T>
m2::RectD GetLimitRect(T && t)
{
  using Type = RemoveCvrefT<T>;
  if constexpr (std::is_same_v<Type, FeatureBuilder>)
    return t.GetLimitRect();
  if constexpr (std::is_same_v<Type, std::vector<m2::PointD>>)
  {
    m2::RectD r;
    for (auto const & p : t)
      r.Add(p);
    return r;
  }
  if constexpr (std::is_same_v<Type, m2::PointD>)
    return m2::RectD(t, t);

  UNREACHABLE();
}

template <typename T, typename F>
bool ForAnyPoint(T && t, F && f)
{
  using Type = RemoveCvrefT<T>;
  if constexpr (std::is_same_v<Type, FeatureBuilder>)
    return t.ForAnyPoint(f);
  if constexpr (std::is_same_v<Type, std::vector<m2::PointD>>)
    return base::AnyOf(t, f);
  if constexpr (std::is_same_v<Type, m2::PointD>)
    return f(t);

  UNREACHABLE();
}

template <typename T, typename F>
void ForEachPoint(T && t, F && f)
{
  using Type = RemoveCvrefT<T>;
  if constexpr (std::is_same_v<Type, FeatureBuilder>)
    t.ForEachPoint(std::forward<F>(f));
  else if constexpr (std::is_same_v<Type, m2::PointD>)
    f(std::forward<T>(t));
  else
    UNREACHABLE();
}

// An implementation for CountriesFilesAffiliation class.
template <typename T>
std::vector<std::string> GetAffiliations(T const & t, borders::CountryPolygonsCollection const & countryPolygonsTree,
                                         bool haveBordersForWholeWorld)
{
  std::vector<std::string> countries;
  std::vector<std::reference_wrapper<borders::CountryPolygons const>> countriesContainer;
  countryPolygonsTree.ForEachCountryInRect(
      GetLimitRect(t), [&](auto const & countryPolygons) { countriesContainer.emplace_back(countryPolygons); });

  // todo(m.andrianov): We need to explore this optimization better. There is a hypothesis: some
  // elements belong to a rectangle, but do not belong to the exact boundary.
  if (haveBordersForWholeWorld && countriesContainer.size() == 1)
  {
    borders::CountryPolygons const & countryPolygons = countriesContainer.front();
    countries.emplace_back(countryPolygons.GetName());
    return countries;
  }

  for (borders::CountryPolygons const & countryPolygons : countriesContainer)
  {
    auto const need = ForAnyPoint(t, [&](auto const & point) { return countryPolygons.Contains(point); });

    if (need)
      countries.emplace_back(countryPolygons.GetName());
  }

  return countries;
}

// An implementation for CountriesFilesIndexAffiliation class.
using IndexSharedPtr = std::shared_ptr<CountriesFilesIndexAffiliation::Tree>;

CountriesFilesIndexAffiliation::Box MakeBox(m2::RectD const & rect)
{
  return {rect.LeftBottom(), rect.RightTop()};
}

std::optional<std::string> IsOneCountryForLimitRect(m2::RectD const & limitRect, IndexSharedPtr const & index)
{
  borders::CountryPolygons const * country = nullptr;
  std::vector<CountriesFilesIndexAffiliation::Value> values;
  auto const bbox = MakeBox(limitRect);
  boost::geometry::index::query(*index, boost::geometry::index::covers(bbox), std::back_inserter(values));
  for (auto const & v : values)
  {
    for (borders::CountryPolygons const & c : v.second)
      if (!country)
        country = &c;
      else if (country != &c)
        return {};
  }
  return country ? country->GetName() : std::optional<std::string>{};
}

template <typename T>
std::vector<std::string> GetHonestAffiliations(T && t, IndexSharedPtr const & index)
{
  std::vector<std::string> affiliations;
  std::unordered_set<borders::CountryPolygons const *> countires;
  ForEachPoint(t, [&](auto const & point)
  {
    std::vector<CountriesFilesIndexAffiliation::Value> values;
    boost::geometry::index::query(*index, boost::geometry::index::covers(point), std::back_inserter(values));
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
          if (cp.Contains(point) && countires.insert(&cp).second)
            affiliations.emplace_back(cp.GetName());
      }
    }
  });

  return affiliations;
}

template <typename T>
std::vector<std::string> GetAffiliations(T && t, IndexSharedPtr const & index)
{
  auto const oneCountry = IsOneCountryForLimitRect(GetLimitRect(t), index);
  return oneCountry ? std::vector<std::string>{*oneCountry} : GetHonestAffiliations(t, index);
}
}  // namespace affiliation

CountriesFilesAffiliation::CountriesFilesAffiliation(std::string const & borderPath, bool haveBordersForWholeWorld)
  : m_countryPolygonsTree(borders::GetOrCreateCountryPolygonsTree(borderPath))
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{}

std::vector<std::string> CountriesFilesAffiliation::GetAffiliations(FeatureBuilder const & fb) const
{
  return affiliation::GetAffiliations(fb, m_countryPolygonsTree, m_haveBordersForWholeWorld);
}

std::vector<std::string> CountriesFilesAffiliation::GetAffiliations(m2::PointD const & point) const
{
  return affiliation::GetAffiliations(point, m_countryPolygonsTree, m_haveBordersForWholeWorld);
}

std::vector<std::string> CountriesFilesAffiliation::GetAffiliations(std::vector<m2::PointD> const & points) const
{
  return affiliation::GetAffiliations(points, m_countryPolygonsTree, m_haveBordersForWholeWorld);
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

  std::lock_guard<std::mutex> lock(cacheMutex);

  auto const it = cache.find(key);
  if (it != std::cend(cache))
  {
    m_index = it->second;
    return;
  }

  auto const net = generator::cells_merger::MakeNet(0.2 /* step */, mercator::Bounds::kMinX, mercator::Bounds::kMinY,
                                                    mercator::Bounds::kMaxX, mercator::Bounds::kMaxY);
  auto const index = BuildIndex(net);
  m_index = index;
  cache.emplace(key, index);
}

std::vector<std::string> CountriesFilesIndexAffiliation::GetAffiliations(FeatureBuilder const & fb) const
{
  return affiliation::GetAffiliations(fb, m_index);
}

std::vector<std::string> CountriesFilesIndexAffiliation::GetAffiliations(m2::PointD const & point) const
{
  return affiliation::GetAffiliations(point, m_index);
}

std::shared_ptr<CountriesFilesIndexAffiliation::Tree> CountriesFilesIndexAffiliation::BuildIndex(
    std::vector<m2::RectD> const & net)
{
  std::unordered_map<borders::CountryPolygons const *, std::vector<m2::RectD>> countriesRects;
  std::mutex countriesRectsMutex;
  std::vector<Value> treeCells;
  std::mutex treeCellsMutex;
  auto const numThreads = GetPlatform().CpuCores();
  {
    base::ComputationalThreadPool pool(numThreads);
    for (auto const & rect : net)
    {
      pool.SubmitWork([&, rect]()
      {
        std::vector<std::reference_wrapper<borders::CountryPolygons const>> countries;
        m_countryPolygonsTree.ForEachCountryInRect(rect,
                                                   [&](auto const & country) { countries.emplace_back(country); });
        if (m_haveBordersForWholeWorld && countries.size() == 1)
        {
          borders::CountryPolygons const & country = countries.front();
          std::lock_guard<std::mutex> lock(countriesRectsMutex);
          countriesRects[&country].emplace_back(rect);
        }
        else
        {
          auto const box = affiliation::MakeBox(rect);
          std::vector<std::reference_wrapper<borders::CountryPolygons const>> interCountries;
          for (borders::CountryPolygons const & cp : countries)
          {
            cp.ForAnyPolygon([&](auto const & polygon)
            {
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
    base::ComputationalThreadPool pool(numThreads);
    for (auto & pair : countriesRects)
    {
      pool.SubmitWork([&, countryPtr{pair.first}, rects{std::move(pair.second)}]() mutable
      {
        generator::cells_merger::CellsMerger merger(std::move(rects));
        auto const merged = merger.Merge();
        for (auto const & rect : merged)
        {
          std::vector<std::reference_wrapper<borders::CountryPolygons const>> interCountries{*countryPtr};
          std::lock_guard<std::mutex> lock(treeCellsMutex);
          treeCells.emplace_back(affiliation::MakeBox(rect), std::move(interCountries));
        }
      });
    }
  }
  return std::make_shared<Tree>(treeCells);
}

SingleAffiliation::SingleAffiliation(std::string const & filename) : m_filename(filename) {}

std::vector<std::string> SingleAffiliation::GetAffiliations(FeatureBuilder const &) const
{
  return {m_filename};
}

bool SingleAffiliation::HasCountryByName(std::string const & name) const
{
  return name == m_filename;
}

std::vector<std::string> SingleAffiliation::GetAffiliations(m2::PointD const &) const
{
  return {m_filename};
}
}  // namespace feature
