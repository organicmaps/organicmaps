#pragma once
#include "routing/routing_exceptions.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/data_source.hpp"

#include "base/lru_cache.hpp"

#include <map>
#include <unordered_map>

namespace routing
{
// Main purpose is to take and hold MwmHandle-s here (readers and caches).
// Routing works in a separate threads and doesn't interfere within route calculation process.
/// @note Should add additional thread's caches in case of concurrent routing process.
class MwmDataSource
{
  DataSource & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIDs;
  std::unordered_map<NumMwmId, MwmSet::MwmHandle> m_handles;

  // Used for FeaturesRoadGraph in openlr only.
  std::map<MwmSet::MwmId, MwmSet::MwmHandle> m_handles2;

  // Cache is important for Cross-Mwm routing, where we need at least 2 sources simultaneously.
  LruCache<MwmSet::MwmId, std::unique_ptr<FeatureSource>> m_featureSources{4};

  MwmSet::MwmHandle const * GetHandleSafe(NumMwmId numMwmId)
  {
    ASSERT(m_numMwmIDs, ());
    auto it = m_handles.find(numMwmId);
    if (it == m_handles.end())
    {
      auto handle = m_dataSource.GetMwmHandleByCountryFile(m_numMwmIDs->GetFile(numMwmId));
      if (!handle.IsAlive())
        return nullptr;

      it = m_handles.emplace(numMwmId, std::move(handle)).first;
    }
    return &(it->second);
  }

public:
  /// @param[in]  numMwmIDs Can be null, if won't call NumMwmId functions.
  MwmDataSource(DataSource & dataSource, std::shared_ptr<NumMwmIds> numMwmIDs)
    : m_dataSource(dataSource)
    , m_numMwmIDs(std::move(numMwmIDs))
  {}

  void FreeHandles()
  {
    m_featureSources.Clear();
    m_handles.clear();
    m_handles2.clear();
  }

  bool IsLoaded(platform::CountryFile const & file) const { return m_dataSource.IsLoaded(file); }

  enum SectionStatus
  {
    MwmNotLoaded,
    SectionExists,
    NoSection,
  };

  MwmSet::MwmHandle const & GetHandle(NumMwmId numMwmId)
  {
    ASSERT(m_numMwmIDs, ());
    auto it = m_handles.find(numMwmId);
    if (it == m_handles.end())
    {
      auto const file = m_numMwmIDs->GetFile(numMwmId);
      auto handle = m_dataSource.GetMwmHandleByCountryFile(file);
      if (!handle.IsAlive())
        MYTHROW(RoutingException, ("Mwm", file, "cannot be loaded."));

      it = m_handles.emplace(numMwmId, std::move(handle)).first;
    }
    return it->second;
  }

  MwmValue const & GetMwmValue(NumMwmId numMwmId) { return *GetHandle(numMwmId).GetValue(); }

  SectionStatus GetSectionStatus(NumMwmId numMwmId, std::string const & section)
  {
    auto const * handle = GetHandleSafe(numMwmId);
    if (!handle)
      return MwmNotLoaded;
    return handle->GetValue()->m_cont.IsExist(section) ? SectionExists : NoSection;
  }

  MwmSet::MwmId GetMwmId(NumMwmId numMwmId) const
  {
    ASSERT(m_numMwmIDs, ());
    return m_dataSource.GetMwmIdByCountryFile(m_numMwmIDs->GetFile(numMwmId));
  }

  template <class FnT>
  void ForEachStreet(FnT && fn, m2::RectD const & rect)
  {
    m_dataSource.ForEachInRect(fn, rect, scales::GetUpperScale());
  }

  MwmSet::MwmHandle const & GetHandle(MwmSet::MwmId const & mwmId)
  {
    if (m_numMwmIDs)
    {
      return GetHandle(m_numMwmIDs->GetId(mwmId.GetInfo()->GetLocalFile().GetCountryFile()));
    }
    else
    {
      auto it = m_handles2.find(mwmId);
      if (it == m_handles2.end())
      {
        auto handle = m_dataSource.GetMwmHandleById(mwmId);
        if (!handle.IsAlive())
          MYTHROW(RoutingException, ("Mwm", mwmId.GetInfo()->GetCountryName(), "cannot be loaded."));

        it = m_handles2.emplace(mwmId, std::move(handle)).first;
      }
      return it->second;
    }
  }

  std::unique_ptr<FeatureType> GetFeature(FeatureID const & id)
  {
    bool found = false;
    auto & ptr = m_featureSources.Find(id.m_mwmId, found);
    if (!found)
      ptr = m_dataSource.CreateFeatureSource(GetHandle(id.m_mwmId));

    /// @todo Should we also retrieve "modified" features here?
    return ptr->GetOriginalFeature(id.m_index);
  }
};
}  // namespace routing
