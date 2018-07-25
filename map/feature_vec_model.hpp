#pragma once

#include "editor/editable_data_source.hpp"

#include "indexer/data_header.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include "coding/reader.hpp"
#include "coding/buffer_reader.hpp"

#include "base/macros.hpp"

#include <functional>

namespace model
{
//#define USE_BUFFER_READER

class FeaturesFetcher : public MwmSet::Observer
  {
  public:
#ifdef USE_BUFFER_READER
    using Reader = BufferReader;
#else
    using Reader = ModelReaderPtr;
#endif

    using MapDeregisteredCallback = std::function<void(platform::LocalCountryFile const &)>;

  private:
    m2::RectD m_rect;

    EditableDataSource m_dataSource;

    MapDeregisteredCallback m_onMapDeregistered;

  public:
    FeaturesFetcher();

    virtual ~FeaturesFetcher();

    void InitClassificator();

    inline void SetOnMapDeregisteredCallback(MapDeregisteredCallback const & callback)
    {
      m_onMapDeregistered = callback;
    }

    /// Registers a new map.
    pair<MwmSet::MwmId, MwmSet::RegResult> RegisterMap(
        platform::LocalCountryFile const & localFile);

    /// Deregisters a map denoted by file from internal records.
    bool DeregisterMap(platform::CountryFile const & countryFile);

    void Clear();

    void ClearCaches();

    inline bool IsLoaded(string const & countryFileName) const
    {
      return m_dataSource.IsLoaded(platform::CountryFile(countryFileName));
    }

    // MwmSet::Observer overrides:
    void OnMapUpdated(platform::LocalCountryFile const & newFile,
                      platform::LocalCountryFile const & oldFile) override;
    void OnMapDeregistered(platform::LocalCountryFile const & localFile) override;

    //bool IsLoaded(m2::PointD const & pt) const;

    /// @name Features enumeration.
    //@{
    void ForEachFeature(m2::RectD const & rect, std::function<void(FeatureType &)> const & fn,
                        int scale) const
    {
      m_dataSource.ForEachInRect(fn, rect, scale);
    }

    void ForEachFeatureID(m2::RectD const & rect, std::function<void(FeatureID const &)> const & fn,
                          int scale) const
    {
      m_dataSource.ForEachFeatureIDInRect(fn, rect, scale);
    }

    template <class ToDo>
    void ReadFeatures(ToDo & toDo, vector<FeatureID> const & features) const
    {
      m_dataSource.ReadFeatures(toDo, features);
    }
    //@}

    DataSource const & GetDataSource() const { return m_dataSource; }
    DataSource & GetDataSource() { return m_dataSource; }
    m2::RectD GetWorldRect() const;
  };
}
