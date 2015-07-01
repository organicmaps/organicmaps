#pragma once

#include "indexer/data_header.hpp"
#include "indexer/index.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include "coding/reader.hpp"
#include "coding/buffer_reader.hpp"

#include "base/macros.hpp"

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

    /// Registers a new map.
    ///
    /// \return A pair of an MwmLock and a flag. MwmLock is locked iff the
    ///         map with fileName was created or already exists. Flag
    ///         is set when the map was registered for a first
    ///         time. Thus, there are three main cases:
    ///
    ///         * the map already exists - returns active lock and unset flag
    ///         * the map was already registered - returns active lock and set flag
    ///         * the map can't be registered - returns inactive lock and unset flag
    WARN_UNUSED_RESULT pair<MwmSet::MwmLock, bool> RegisterMap(string const & file);

    /// Deregisters a map denoted by file from internal records.
    void DeregisterMap(string const & file);

    /// Deregisters all registered maps.
    void DeregisterAllMaps();

    /// Deletes all files related to map denoted by file.
    ///
    /// \return True if a map was successfully deleted.
    bool DeleteMap(string const & file);

    /// Replaces a map file corresponding to fileName with a new one, when
    /// it's possible - no clients of the map file. Otherwise, update
    /// will be delayed.
    ///
    /// \return * the map file have been updated - returns active lock and
    ///           UPDATE_STATUS_OK
    ///         * update is delayed because the map is busy - returns active lock and
    ///           UPDATE_STATUS_UPDATE_DELAYED
    ///         * the file isn't suitable for update - returns inactive lock and
    ///           UPDATE_STATUS_BAD_FILE
    WARN_UNUSED_RESULT pair<MwmSet::MwmLock, Index::UpdateStatus> UpdateMap(string const & file);
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
