#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "geometry/rect2d.hpp"

#include "std/cstdint.hpp"
#include "std/deque.hpp"
#include "std/map.hpp"
#include "std/unique_ptr.hpp"

namespace my
{
class Cancellable;
};

namespace search
{
namespace v2
{
class MwmContext;

// This class represents a simple cache of features in rects for all mwms.
//
// *NOTE* This class is not thread-safe.
class GeometryCache
{
public:
  // |maxNumEntries| denotes the maximum number of rectangles that
  // will be cached for each mwm individually.
  GeometryCache(size_t maxNumEntries, my::Cancellable const & cancellable);

  // Returns (hopefully, cached) list of features in a given
  // rect. Note that return value may be invalidated on next calls to
  // this method.
  coding::CompressedBitVector const * Get(MwmContext const & context, m2::RectD const & rect,
                                          int scale);

  inline void Clear() { m_entries.clear(); }

private:
  struct Entry
  {
    Entry();

    Entry(m2::RectD const & lowerBound, m2::RectD const & upperBound,
          unique_ptr<coding::CompressedBitVector> cbv, int scale);

    bool Matches(m2::RectD const & rect, int scale) const;

    m2::RectD m_lowerBound;
    m2::RectD m_upperBound;
    unique_ptr<coding::CompressedBitVector> m_cbv;
    int m_scale;
  };

  map<MwmSet::MwmId, deque<Entry>> m_entries;
  size_t const m_maxNumEntries;
  my::Cancellable const & m_cancellable;
};
}  // namespace v2
}  // namespace search
