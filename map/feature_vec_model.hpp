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
  class FeatureVector
  {
    vector<FeatureBuilderType> m_vec;
    m2::RectD m_rect;

  public:
    template <class TParam, class ToDo>
    void ForEachFeature(TParam const &, ToDo & toDo)
    {
      std::for_each(m_vec.begin(), m_vec.end(), toDo);
    }

    m2::RectD GetWorldRect() const { return m_rect; }
  };

//#define USE_BUFFER_READER

  class FeaturesFetcher
  {
    m2::RectD m_rect;

#ifdef USE_BUFFER_READER
    typedef BufferReader reader_t;
#else
    typedef FileReader reader_t;
#endif

    typedef Index<reader_t, reader_t>::Type index_t;

    index_t m_multiIndex;

  public:
    void InitClassificator();

    void AddMap(string const & dataPath, string const & indexPath);
    void RemoveMap(string const & dataPath);
    void Clean();

    // process features by param type indices
    template <class ToDo>
    void ForEachFeature(m2::RectD const & rect, ToDo toDo) const
    {
      m_multiIndex.ForEachInViewport(toDo, rect);
      // Uncomment to traverse all features (SLOW!!):
      // m_multiIndex.ForEachInScale(toDo, GetScaleLevel(rect));
    }

    template <class ToDo>
    void ForEachFeatureWithScale(m2::RectD const & rect, ToDo toDo, int scale) const
    {
      m_multiIndex.ForEachInRect(toDo, rect, scale);
    }

    void AddWorldRect(m2::RectD const & r) { m_rect.Add(r); }
    m2::RectD GetWorldRect() const { return m_rect; }
  };
}

#include "../base/stop_mem_debug.hpp"
