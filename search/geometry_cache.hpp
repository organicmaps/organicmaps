#pragma once

#include "search/cbv.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <utility>

namespace base
{
class Cancellable;
}

namespace search
{
class MwmContext;

// This class represents a simple cache of features in rects for all mwms.
//
// *NOTE* This class is not thread-safe.
class GeometryCache
{
public:
  virtual ~GeometryCache() = default;

  // Returns (hopefully, cached) list of features in a given
  // rect. Note that return value may be invalidated on next calls to
  // this method.
  virtual CBV Get(MwmContext const & context, m2::RectD const & rect, int scale) = 0;

  inline void Clear() { m_entries.clear(); }

protected:
  struct Entry
  {
    m2::RectD m_rect;
    CBV m_cbv;
    int m_scale = 0;
  };

  // |maxNumEntries| denotes the maximum number of rectangles that
  // will be cached for each mwm individually.
  GeometryCache(size_t maxNumEntries, base::Cancellable const & cancellable);

  template <typename Pred>
  std::pair<Entry &, bool> FindOrCreateEntry(MwmSet::MwmId const & id, Pred && pred)
  {
    auto & entries = m_entries[id];
    auto it = std::find_if(entries.begin(), entries.end(), std::forward<Pred>(pred));
    if (it != entries.end())
    {
      if (it != entries.begin())
        iter_swap(entries.begin(), it);
      return std::pair<Entry &, bool>(entries.front(), false);
    }

    entries.emplace_front();
    if (entries.size() == m_maxNumEntries + 1)
      entries.pop_back();

    ASSERT_LESS_OR_EQUAL(entries.size(), m_maxNumEntries, ());
    ASSERT(!entries.empty(), ());
    return std::pair<Entry &, bool>(entries.front(), true);
  }

  void InitEntry(MwmContext const & context, m2::RectD const & rect, int scale, Entry & entry);

  std::map<MwmSet::MwmId, std::deque<Entry>> m_entries;
  size_t const m_maxNumEntries;
  base::Cancellable const & m_cancellable;
};

class PivotRectsCache : public GeometryCache
{
public:
  PivotRectsCache(size_t maxNumEntries, base::Cancellable const & cancellable, double maxRadiusMeters);

  // GeometryCache overrides:
  CBV Get(MwmContext const & context, m2::RectD const & rect, int scale) override;

private:
  double const m_maxRadiusMeters;
};

class LocalityRectsCache : public GeometryCache
{
public:
  LocalityRectsCache(size_t maxNumEntries, base::Cancellable const & cancellable);

  // GeometryCache overrides:
  CBV Get(MwmContext const & context, m2::RectD const & rect, int scale) override;
};
}  // namespace search
