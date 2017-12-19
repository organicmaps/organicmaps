#pragma once

#include "transit/transit_types.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct TransitFeatureInfo
{
  bool m_isGate = false;
  std::string m_gateSymbolName;
  std::string m_title;
  m2::PointD m_point;
};

using TransitFeaturesInfo = std::map<FeatureID, TransitFeatureInfo>;

using TransitStopsInfo = std::map<routing::transit::StopId, routing::transit::Stop>;
using TransitTransfersInfo = std::map<routing::transit::TransferId, routing::transit::Transfer>;
using TransitShapesInfo = std::map<routing::transit::ShapeId, routing::transit::Shape>;
using TransitLinesInfo = std::map<routing::transit::LineId, routing::transit::Line>;
using TransitNetworksInfo = std::map<routing::transit::NetworkId, routing::transit::Network>;

struct TransitDisplayInfo
{
  TransitNetworksInfo m_networks;
  TransitLinesInfo m_lines;
  TransitStopsInfo m_stops;
  TransitTransfersInfo m_transfers;
  TransitShapesInfo m_shapes;
  TransitFeaturesInfo m_features;
};

template <typename T> using TReadCallback = std::function<void (T const &)>;
using TReadFeaturesFn = std::function<void (TReadCallback<FeatureType> const & , std::vector<FeatureID> const &)>;

class ReadTransitTask: public threads::IRoutine
{
public:
  ReadTransitTask(Index & index,
                  TReadFeaturesFn const & readFeaturesFn)
    : m_index(index), m_readFeaturesFn(readFeaturesFn)
  {}

  void Init(uint64_t id, MwmSet::MwmId const & mwmId,
            std::unique_ptr<TransitDisplayInfo> transitInfo = nullptr);
  uint64_t GetId() const { return m_id; }
  bool GetSuccess() const { return m_success; }

  void Do() override;
  void Reset() override;

  std::unique_ptr<TransitDisplayInfo> && GetTransitInfo();

private:
  template <typename T, typename TID>
  void FillItemsByIdMap(std::vector<T> const & items, std::map<TID, T> & itemsById)
  {
    for (auto const & item : items)
    {
      if (!m_loadSubset)
      {
        itemsById.insert(make_pair(item.GetId(), item));
      }
      else
      {
        auto it = itemsById.find(item.GetId());
        if (it != itemsById.end())
          it->second = item;
      }
    }
  };

  Index & m_index;
  TReadFeaturesFn m_readFeaturesFn;

  uint64_t m_id = 0;
  MwmSet::MwmId m_mwmId;
  std::unique_ptr<TransitDisplayInfo> m_transitInfo;

  bool m_loadSubset = false;
  // Sets to true if Do() method was executed successfully.
  bool m_success = false;
};

using TransitDisplayInfos = std::map<MwmSet::MwmId, unique_ptr<TransitDisplayInfo>>;

class TransitReadManager
{
public:
  TransitReadManager(Index & index, TReadFeaturesFn const & readFeaturesFn);
  ~TransitReadManager();

  void Start();
  void Stop();

  bool GetTransitDisplayInfo(TransitDisplayInfos & transitDisplayInfos);

  // TODO(@darina) Clear cache for deleted mwm.
  //void OnMwmDeregistered(MwmSet::MwmId const & mwmId);

private:
  void OnTaskCompleted(threads::IRoutine * task);

  std::unique_ptr<threads::ThreadPool> m_threadsPool;

  std::mutex m_mutex;
  std::condition_variable m_event;

  uint64_t m_nextTasksGroupId = 0;
  std::map<uint64_t, size_t> m_tasksGroups;

  Index & m_index;
  TReadFeaturesFn m_readFeaturesFn;
  // TODO(@darina) In case of reading the whole mwm transit section, save it in the cache for transit scheme rendering.
  //TransitDisplayInfos m_transitDisplayCache;
};
