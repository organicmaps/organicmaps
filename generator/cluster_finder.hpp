#pragma once

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include <functional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace generator
{
// The class ClustersFinder finds clusters of objects for which IsSameFunc returns true.
// RadiusFunc should return the same radius for all objects in one cluster.
template <class T>
class ClustersFinder
{
public:
  using PtrT = T const *;
  using RadiusFunc = std::function<double(T const &)>;
  using IsSameFunc = std::function<bool(T const &, T const &)>;

  ClustersFinder(std::vector<T> const & container, RadiusFunc radiusFunc, IsSameFunc isSameFunc)
    : m_container(container)
    , m_radiusFunc(std::move(radiusFunc))
    , m_isSameFunc(std::move(isSameFunc))
  {
    for (auto const & e : m_container)
      m_tree.Add(&e);
  }

  std::vector<std::vector<PtrT>> Find() const
  {
    std::vector<std::vector<PtrT>> clusters;
    std::set<PtrT> unviewed;
    for (auto const & e : m_container)
      unviewed.insert(&e);

    while (!unviewed.empty())
    {
      auto const it = *std::cbegin(unviewed);
      clusters.emplace_back(FindOneCluster(it, unviewed));
    }
    return clusters;
  }

private:
  struct TraitsDef
  {
    m2::RectD const LimitRect(PtrT p) const { return GetLimitRect(*p); }
  };

  std::vector<PtrT> FindOneCluster(PtrT p, std::set<PtrT> & unviewed) const
  {
    std::vector<PtrT> cluster{p};
    std::queue<PtrT> queue;
    queue.emplace(p);
    unviewed.erase(p);
    while (!queue.empty())
    {
      auto const current = queue.front();
      queue.pop();
      auto const queryBbox = GetBboxFor(current);
      m_tree.ForEachInRect(queryBbox, [&](PtrT candidate)
      {
        if (unviewed.count(candidate) == 0 || !m_isSameFunc(*p, *candidate))
          return;

        unviewed.erase(candidate);
        queue.emplace(candidate);
        cluster.emplace_back(candidate);
      });
    }

    return cluster;
  }

  m2::RectD GetBboxFor(PtrT p) const
  {
    m2::RectD bbox;
    auto const dist = m_radiusFunc(*p);
    GetLimitRect(*p).ForEachCorner([&](auto const & p) { bbox.Add(mercator::RectByCenterXYAndSizeInMeters(p, dist)); });
    return bbox;
  }

  std::vector<T> const & m_container;
  RadiusFunc m_radiusFunc;
  IsSameFunc m_isSameFunc;
  m4::Tree<PtrT, TraitsDef> m_tree;
};

/// @return Vector of equal place clusters, like pointers from input \a container.
template <class T, class RadiusFnT, class IsSameFnT>
std::vector<std::vector<T const *>> GetClusters(std::vector<T> const & container, RadiusFnT && radiusFunc,
                                                IsSameFnT && isSameFunc)
{
  return ClustersFinder<T>(container, std::forward<RadiusFnT>(radiusFunc), std::forward<IsSameFnT>(isSameFunc)).Find();
}
}  // namespace generator
