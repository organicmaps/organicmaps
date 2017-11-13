#pragma once

#include "routing_common/transit_serdes.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TransitReader
{
public:
  TransitReader(Index & index)
    : m_index(index)
  {}

  void ReadStops(MwmSet::MwmId const & mwmId, std::vector<routing::transit::Stop> & stops);
  void ReadShapes(MwmSet::MwmId const & mwmId, std::vector<routing::transit::Shape> & shapes);
  void ReadTransfers(MwmSet::MwmId const & mwmId, std::vector<routing::transit::Transfer> & transfers);
  void ReadLines(MwmSet::MwmId const & mwmId, std::vector<routing::transit::Line> & lines);
  void ReadNetworks(MwmSet::MwmId const & mwmId, std::vector<routing::transit::Network> & networks);

private:
  using GetItemsOffsetFn = std::function<uint32_t (routing::transit::TransitHeader const & header)>;
  template <typename T>
  void ReadTable(MwmSet::MwmId const & mwmId, GetItemsOffsetFn getItemsOffsetFn, std::vector<T> & items)
  {
    items.clear();
    try
    {
      MwmSet::MwmHandle handle = m_index.GetMwmHandleById(mwmId);
      if (!handle.IsAlive())
      {
        LOG(LWARNING, ("Can't get mwm handle for", mwmId));
        return;
      }
      MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
      if (!mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
      {
        LOG(LWARNING, ("Can't get transit for", mwmId));
        return;
      }
      FilesContainerR::TReader reader = mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG);
      ReaderSource<FilesContainerR::TReader> src(reader);

      routing::transit::FixedSizeDeserializer<ReaderSource<FilesContainerR::TReader>> fixedSizeDeserializer(src);
      routing::transit::TransitHeader header;
      fixedSizeDeserializer(header);

      auto const offset = getItemsOffsetFn(header);
      CHECK_GREATER_OR_EQUAL(offset, src.Pos(), ("Wrong section format."));
      src.Skip(offset - src.Pos());

      routing::transit::Deserializer<ReaderSource<FilesContainerR::TReader>> deserializer(src);
      deserializer(items);
    }
    catch (Reader::OpenException const & e)
    {
      LOG(LERROR, ("Error while reading", TRANSIT_FILE_TAG, "section.", e.Msg()));
      throw;
    }
  }

  Index & m_index;
};

struct TransitFeatureInfo
{
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
  ReadTransitTask(TransitReader & reader,
                  TReadFeaturesFn const & readFeaturesFn)
    : m_transitReader(reader), m_readFeaturesFn(readFeaturesFn)
  {}

  void Init(uint64_t id, MwmSet::MwmId const & mwmId, std::unique_ptr<TransitDisplayInfo> && transitInfo = nullptr);
  uint64_t GetId() const { return m_id; }

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

  TransitReader & m_transitReader;
  TReadFeaturesFn m_readFeaturesFn;

  uint64_t m_id;
  MwmSet::MwmId m_mwmId;
  std::unique_ptr<TransitDisplayInfo> m_transitInfo;

  bool m_loadSubset = false;
};

using TransitDisplayInfos = std::map<MwmSet::MwmId, unique_ptr<TransitDisplayInfo>>;

class TransitReadManager
{
public:
  TransitReadManager(Index & index, TReadFeaturesFn const & readFeaturesFn);
  ~TransitReadManager();

  void Start();
  void Stop();

  void GetTransitDisplayInfo(TransitDisplayInfos & transitDisplayInfos);

  // TODO(@darina) Clear cache for deleted mwm.
  //void OnMwmDeregistered(MwmSet::MwmId const & mwmId);

private:
  void OnTaskCompleted(threads::IRoutine * task);

  std::unique_ptr<threads::ThreadPool> m_threadsPool;

  std::mutex m_mutex;
  std::condition_variable m_event;

  std::atomic<uint64_t> m_nextTasksGroupId;
  std::map<uint64_t, size_t> m_tasksGroups;

  TransitReader m_transitReader;
  TReadFeaturesFn m_readFeaturesFn;

  // TODO(@darina) In case of reading the whole mwm transit section, save it in the cache for transit scheme rendering.
  //TransitDisplayInfos m_transitDisplayCache;
};
