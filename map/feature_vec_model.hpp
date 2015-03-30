#pragma once

#include "../indexer/data_header.hpp"
#include "../indexer/index.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"

#include "../coding/reader.hpp"
#include "../coding/buffer_reader.hpp"

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

    Index m_multiIndex;

  public:
    void InitClassificator();

    /// @param[in] file Name of mwm file with extension.
    //{@
    /// @return True when map was successfully registered, also, sets
    ///         version to the file format version.
    bool RegisterMap(string const & file, feature::DataHeader::Version & version);

    /// Deregisters map denoted by file from internal records.
    void DeregisterMap(string const & file);

    /// Deregisters all registered maps.
    void DeregisterAllMaps();

    /// Deletes all files related to map denoted by file.
    ///
    /// \return True if map was successfully deleted.
    bool DeleteMap(string const & file);

    /// Tries to update map denoted by file. If map is used right now,
    /// update will be delayed.
    ///
    /// \return True when map was successfully updated, false when update
    ///         was delayed or when update's version is not good.
    bool UpdateMap(string const & file, m2::RectD & rect);
    //@}

    //void Clean();
    void ClearCaches();

    inline bool IsLoaded(string const & fName) const
    {
      return m_multiIndex.IsLoaded(fName);
    }

    //bool IsLoaded(m2::PointD const & pt) const;

    /// @name Features enumeration.
    //@{
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

    template <class ToDo>
    void ForEachFeatureID(m2::RectD const & rect, ToDo & toDo, int scale) const
    {
      m_multiIndex.ForEachFeatureIDInRect(toDo, rect, scale);
    }

    template <class ToDo>
    void ReadFeatures(ToDo & toDo, vector<FeatureID> const & features) const
    {
      m_multiIndex.ReadFeatures(toDo, features);
    }
    //@}

    Index const & GetIndex() const { return m_multiIndex; }
    m2::RectD GetWorldRect() const;
  };
}
