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
template <typename T, template<typename, typename> class Container, typename Alloc = std::allocator<T>>
class ClustersFinder
{
public:
  using RadiusFunc = std::function<double(T const &)>;
  using IsSameFunc = std::function<bool(T const &, T const &)>;

  ClustersFinder(Container<T, Alloc> && container, RadiusFunc const & radiusFunc,
                 IsSameFunc const & isSameFunc)
    : m_container(std::move(container)), m_radiusFunc(radiusFunc), m_isSameFunc(isSameFunc)
  {
    for (auto const & e : m_container)
      m_tree.Add(&e);
  }

  std::vector<std::vector<T>> Find() const
  {
    std::vector<std::vector<T>> clusters;
    std::set<ConstIterator> unviewed;
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
  using ConstIterator = T const *;

  struct TraitsDef
  {
    m2::RectD const LimitRect(ConstIterator const & it) const { return GetLimitRect(*it); }
  };

  std::vector<T> FindOneCluster(ConstIterator const & it, std::set<ConstIterator> & unviewed) const
  {
    std::vector<T> cluster{*it};
    std::queue<ConstIterator> queue;
    queue.emplace(it);
    unviewed.erase(it);
    while (!queue.empty())
    {
      auto const current = queue.front();
      queue.pop();
      auto const queryBbox = GetBboxFor(current);
      m_tree.ForEachInRect(queryBbox, [&](auto const & candidate) {
        if (unviewed.count(candidate) == 0 || !m_isSameFunc(*it, *candidate))
          return;

        unviewed.erase(candidate);
        queue.emplace(candidate);
        cluster.emplace_back(*candidate);
      });
    }

    return cluster;
  }

  m2::RectD GetBboxFor(ConstIterator const & it) const
  {
    m2::RectD bbox;
    auto const dist = m_radiusFunc(*it);
    GetLimitRect(*it).ForEachCorner([&](auto const & p) {
      bbox.Add(mercator::RectByCenterXYAndSizeInMeters(p, dist));
    });

    return bbox;
  }

  Container<T, Alloc> m_container;
  RadiusFunc m_radiusFunc;
  IsSameFunc m_isSameFunc;
  m4::Tree<ConstIterator, TraitsDef> m_tree;
};

template <typename T, template<typename, typename> class Container, typename Alloc = std::allocator<T>>
std::vector<std::vector<T>> GetClusters(
    Container<T, Alloc> && container,
    typename ClustersFinder<T, Container, Alloc>::RadiusFunc const & radiusFunc,
    typename ClustersFinder<T, Container, Alloc>::IsSameFunc const & isSameFunc)
{
  return ClustersFinder<T, Container, Alloc>(std::forward<Container<T, Alloc>>(container),
                                             radiusFunc, isSameFunc).Find();
}
}  // namespace generator
