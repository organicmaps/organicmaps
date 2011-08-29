#pragma once

#include "../indexer/index.hpp"
#include "../indexer/scales.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"

#include "../coding/reader.hpp"
#include "../coding/buffer_reader.hpp"

#include "../base/start_mem_debug.hpp"


namespace model
{
//#define USE_BUFFER_READER

  class FeaturesFetcher
  {
  public:
#ifdef USE_BUFFER_READER
    typedef BufferReader ReaderT;
#else
    typedef ModelReaderPtr ReaderT;
#endif

  private:
    m2::RectD m_rect;

    typedef Index<ReaderT>::Type index_t;

    index_t m_multiIndex;

  public:
    void InitClassificator();

    void AddMap(string const & file);
    void RemoveMap(string const & fName);
    void Clean();
    void ClearCaches();

    /// @name Features enumeration.
    //@{
    template <class ToDo>
    void ForEachFeature(m2::RectD const & rect, ToDo & toDo) const
    {
      ForEachFeature(rect, toDo, scales::GetScaleLevel(rect));
    }

    template <class ToDo>
    void ForEachFeature(m2::RectD const & rect, ToDo & toDo, int scale) const
    {
      m_multiIndex.ForEachInRect(toDo, rect, scale);
    }

    template <class ToDo>
    void ForEachFeature_TileDrawing(m2::RectD const & rect, ToDo & toDo, int scale) const
    {
      m_multiIndex.ForEachInRect_TileDrawing(toDo, rect, scale);
    }
    //@}

    index_t const & GetIndex() const { return m_multiIndex; }

    void AddWorldRect(m2::RectD const & r) { m_rect.Add(r); }
    m2::RectD GetWorldRect() const;
  };
}

#include "../base/stop_mem_debug.hpp"
