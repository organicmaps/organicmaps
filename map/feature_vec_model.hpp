#pragma once

#include "../indexer/feature.hpp"
#include "../indexer/features_vector.hpp"
#include "../indexer/index.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/buffer_reader.hpp"

#include "../base/start_mem_debug.hpp"


namespace model
{
//#define USE_BUFFER_READER

  class FeaturesFetcher
  {
    m2::RectD m_rect;

#ifdef USE_BUFFER_READER
    typedef BufferReader reader_t;
#else
    typedef FileReader reader_t;
#endif

    typedef Index<reader_t>::Type index_t;

    index_t m_multiIndex;

    // Cached query, which stores several caches for the index.
    mutable index_t::Query m_multiIndexQuery;

  public:

    void InitClassificator();

    void AddMap(string const & fName);
    void RemoveMap(string const & fName);
    void Clean();

    // process features by param type indices
    template <class ToDo>
    void ForEachFeature(m2::RectD const & rect, ToDo toDo) const
    {
      m_multiIndexQuery.Clear();
      m_multiIndex.ForEachInViewport(toDo, rect, m_multiIndexQuery);
      // Uncomment to traverse all features (SLOW!!):
      // m_multiIndex.ForEachInScale(toDo, GetScaleLevel(rect));
    }

    template <class ToDo>
    void ForEachFeatureWithScale(m2::RectD const & rect, ToDo toDo, int scale) const
    {
      m_multiIndex.ForEachInRect(toDo, rect, scale);
    }

    void AddWorldRect(m2::RectD const & r) { m_rect.Add(r); }
    m2::RectD GetWorldRect() const;
  };
}

#include "../base/stop_mem_debug.hpp"
