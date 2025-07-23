#include "map/transit/transit_reader.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/visual_params.hpp"

#include "transit/transit_graph_data.hpp"
#include "transit/transit_version.hpp"

#include "indexer/data_source.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"  // needed despite of IDE warning
#include "indexer/feature_algo.hpp"

#include "coding/reader.hpp"

#include <algorithm>
#include <chrono>
#include <memory>

using namespace std;
using namespace std::chrono;

namespace
{
int constexpr kMinSchemeZoomLevel = 10;
size_t constexpr kMaxTransitCacheSizeBytes = 5 /* Mb */ * 1024 * 1024;

size_t CalculateCacheSize(TransitDisplayInfo const & transitInfo)
{
  size_t const kSegmentSize = 72;
  size_t cacheSize = 0;
  for (auto const & shape : transitInfo.m_shapesSubway)
    cacheSize += shape.second.GetPolyline().size() * kSegmentSize;
  return cacheSize;
}
}  // namespace

// ReadTransitTask --------------------------------------------------------------------------------
void ReadTransitTask::Init(uint64_t id, MwmSet::MwmId const & mwmId, unique_ptr<TransitDisplayInfo> transitInfo)
{
  m_id = id;
  m_mwmId = mwmId;
  if (transitInfo == nullptr)
  {
    m_loadSubset = false;
    m_transitInfo = make_unique<TransitDisplayInfo>();
  }
  else
  {
    m_loadSubset = true;
    m_transitInfo = std::move(transitInfo);
  }
  m_success = false;
}

void ReadTransitTask::Do()
{
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleById(m_mwmId);
  if (!handle.IsAlive())
  {
    // It's possible that mwm handle is not alive because mwm may be removed after
    // transit route is built but before this task is executed.
    LOG(LWARNING, ("Can't get mwm handle for", m_mwmId));
    m_success = false;
    return;
  }
  MwmValue const & mwmValue = *handle.GetValue();
  if (!m_loadSubset && !mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
  {
    m_success = true;
    return;
  }
  CHECK(mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG),
        ("No transit section in mwm, but transit route was built with it. mwmId:", m_mwmId));

  auto reader = mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG);
  CHECK(reader.GetPtr() != nullptr, ());

  auto const transitHeaderVersion = ::transit::GetVersion(*reader.GetPtr());
  if (transitHeaderVersion == ::transit::TransitVersion::OnlySubway)
  {
    m_transitInfo->m_transitVersion = ::transit::TransitVersion::OnlySubway;

    routing::transit::GraphData graphData;
    graphData.DeserializeForRendering(*reader.GetPtr());

    FillItemsByIdMap(graphData.GetStops(), m_transitInfo->m_stopsSubway);
    for (auto const & stop : m_transitInfo->m_stopsSubway)
    {
      if (stop.second.GetFeatureId() != kInvalidFeatureId)
      {
        auto const featureId = FeatureID(m_mwmId, stop.second.GetFeatureId());
        m_transitInfo->m_features[featureId] = {};
      }

      if (m_loadSubset && (stop.second.GetTransferId() != routing::transit::kInvalidTransferId))
        m_transitInfo->m_transfersSubway[stop.second.GetTransferId()] = {};
    }
    FillItemsByIdMap(graphData.GetTransfers(), m_transitInfo->m_transfersSubway);
    FillItemsByIdMap(graphData.GetLines(), m_transitInfo->m_linesSubway);
    FillItemsByIdMap(graphData.GetShapes(), m_transitInfo->m_shapesSubway);
  }
  else if (transitHeaderVersion == ::transit::TransitVersion::AllPublicTransport)
  {
    m_transitInfo->m_transitVersion = ::transit::TransitVersion::AllPublicTransport;

    ::transit::experimental::TransitData transitData;
    transitData.DeserializeForRendering(*reader.GetPtr());

    FillItemsByIdMap(transitData.GetStops(), m_transitInfo->m_stopsPT);

    for (auto const & edge : transitData.GetEdges())
    {
      ::transit::EdgeId const edgeId(edge.GetStop1Id(), edge.GetStop2Id(), edge.GetLineId());
      ::transit::EdgeData const edgeData(edge.GetShapeLink(), edge.GetWeight());

      if (!m_loadSubset)
      {
        m_transitInfo->m_edgesPT.emplace(edgeId, edgeData);
      }
      else
      {
        auto it = m_transitInfo->m_edgesPT.find(edgeId);
        if (it != m_transitInfo->m_edgesPT.end())
          it->second = edgeData;
      }
    }

    for (auto const & stop : m_transitInfo->m_stopsPT)
    {
      if (stop.second.GetFeatureId() != kInvalidFeatureId)
      {
        auto const featureId = FeatureID(m_mwmId, stop.second.GetFeatureId());
        m_transitInfo->m_features[featureId] = {};
      }

      if (m_loadSubset && !stop.second.GetTransferIds().empty())
        for (auto const transferId : stop.second.GetTransferIds())
          m_transitInfo->m_transfersPT[transferId] = {};
    }

    FillItemsByIdMap(transitData.GetTransfers(), m_transitInfo->m_transfersPT);
    FillItemsByIdMap(transitData.GetShapes(), m_transitInfo->m_shapesPT);
    FillLinesAndRoutes(transitData);
  }
  else
  {
    CHECK(false, (transitHeaderVersion));
  }

  vector<FeatureID> features;
  for (auto & id : m_transitInfo->m_features)
    features.push_back(id.first);
  sort(features.begin(), features.end());

  m_readFeaturesFn([this](FeatureType & ft)
  {
    auto & featureInfo = m_transitInfo->m_features[ft.GetID()];

    featureInfo.m_title = ft.GetReadableName();

    if (featureInfo.m_isGate)
    {
      // TODO(pastk): there should be a simpler way to just get a symbol name.
      df::Stylist stylist(ft, 19, 0);
      if (stylist.m_symbolRule != nullptr)
        featureInfo.m_gateSymbolName = stylist.m_symbolRule->name();
    }
    featureInfo.m_point = feature::GetCenter(ft);
  }, features);

  m_success = true;
}

void ReadTransitTask::Reset()
{
  m_transitInfo.reset();
  m_id = 0;
  IRoutine::Reset();
}

unique_ptr<TransitDisplayInfo> && ReadTransitTask::GetTransitInfo()
{
  return std::move(m_transitInfo);
}

void ReadTransitTask::FillLinesAndRoutes(::transit::experimental::TransitData const & transitData)
{
  auto const & routes = transitData.GetRoutes();
  auto const & linesMeta = transitData.GetLinesMetadata();

  for (auto const & line : transitData.GetLines())
  {
    ::transit::TransitId const routeId = line.GetRouteId();
    auto const itRoute = ::transit::FindById(routes, routeId);
    auto const itLineMetadata = ::transit::FindById(linesMeta, line.GetId(), false /*exists*/);

    if (m_loadSubset)
    {
      auto it = m_transitInfo->m_linesPT.find(line.GetId());

      if (it != m_transitInfo->m_linesPT.end())
      {
        it->second = line;
        m_transitInfo->m_routesPT.emplace(routeId, *itRoute);
        if (itLineMetadata != linesMeta.end())
          m_transitInfo->m_linesMetadataPT.emplace(itLineMetadata->GetId(), *itLineMetadata);
      }
    }
    else
    {
      m_transitInfo->m_linesPT.emplace(line.GetId(), line);
      m_transitInfo->m_routesPT.emplace(routeId, *itRoute);
      if (itLineMetadata != linesMeta.end())
        m_transitInfo->m_linesMetadataPT.emplace(itLineMetadata->GetId(), *itLineMetadata);
    }
  }
}

TransitReadManager::TransitReadManager(DataSource & dataSource, TReadFeaturesFn const & readFeaturesFn,
                                       GetMwmsByRectFn const & getMwmsByRectFn)
  : m_dataSource(dataSource)
  , m_readFeaturesFn(readFeaturesFn)
  , m_getMwmsByRectFn(getMwmsByRectFn)
{
  Start();
}

TransitReadManager::~TransitReadManager()
{
  Stop();
}

void TransitReadManager::Start()
{
  if (m_threadsPool != nullptr)
    return;

  using namespace placeholders;
  uint8_t constexpr kThreadsCount = 1;
  m_threadsPool = make_unique<base::ThreadPool>(kThreadsCount, bind(&TransitReadManager::OnTaskCompleted, this, _1));
}

void TransitReadManager::Stop()
{
  if (m_threadsPool != nullptr)
    m_threadsPool->Stop();
  m_threadsPool.reset();
}

void TransitReadManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void TransitReadManager::EnableTransitSchemeMode(bool enable)
{
  ChangeState(enable ? TransitSchemeState::Enabled : TransitSchemeState::Disabled);
  if (m_isSchemeMode == enable)
    return;
  m_isSchemeMode = enable;

  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableTransitScheme, enable);

  if (m_isSchemeModeBlocked)
    return;

  if (!m_isSchemeMode)
  {
    m_lastActiveMwms.clear();
    m_mwmCache.clear();
    m_cacheSize = 0;
  }
  else
  {
    Invalidate();
  }
}

void TransitReadManager::BlockTransitSchemeMode(bool isBlocked)
{
  if (m_isSchemeModeBlocked == isBlocked)
    return;

  m_isSchemeModeBlocked = isBlocked;

  if (!m_isSchemeMode)
    return;

  if (m_isSchemeModeBlocked)
  {
    m_drapeEngine.SafeCall(&df::DrapeEngine::ClearAllTransitSchemeCache);

    m_lastActiveMwms.clear();
    m_mwmCache.clear();
    m_cacheSize = 0;
  }
  else
  {
    Invalidate();
  }
}

void TransitReadManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = {screen, true /* initialized */};

  if (!m_isSchemeMode || m_isSchemeModeBlocked)
    return;

  if (df::GetDrawTileScale(screen) < kMinSchemeZoomLevel)
  {
    ChangeState(TransitSchemeState::Enabled);
    return;
  }

  auto mwms = m_getMwmsByRectFn(screen.ClipRect());

  m_lastActiveMwms.clear();
  auto const currentTime = steady_clock::now();

  TransitDisplayInfos newTransitData;
  for (auto const & mwmId : mwms)
  {
    if (!mwmId.IsAlive())
      continue;
    m_lastActiveMwms.insert(mwmId);
    auto it = m_mwmCache.find(mwmId);
    if (it == m_mwmCache.end())
    {
      newTransitData[mwmId] = {};
      m_mwmCache.insert(make_pair(mwmId, CacheEntry(currentTime)));
    }
    else
    {
      it->second.m_lastActiveTime = currentTime;
    }
  }

  if (!newTransitData.empty())
  {
    GetTransitDisplayInfo(newTransitData);

    TransitDisplayInfos validTransitData;
    for (auto & [mwmId, transitInfo] : newTransitData)
    {
      if (!transitInfo)
        continue;

      if (transitInfo->m_transitVersion == ::transit::TransitVersion::OnlySubway && transitInfo->m_linesSubway.empty())
        continue;

      if (transitInfo->m_transitVersion == ::transit::TransitVersion::AllPublicTransport &&
          transitInfo->m_linesPT.empty())
        continue;

      auto it = m_mwmCache.find(mwmId);

      it->second.m_isLoaded = true;

      auto const dataSize = CalculateCacheSize(*transitInfo);
      it->second.m_dataSize = dataSize;
      m_cacheSize += dataSize;

      validTransitData[mwmId] = std::move(transitInfo);
    }

    if (!validTransitData.empty())
    {
      ShrinkCacheToAllowableSize();
      m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateTransitScheme, std::move(validTransitData));
    }
  }

  bool hasData = m_lastActiveMwms.empty();
  for (auto const & mwmId : m_lastActiveMwms)
  {
    if (m_mwmCache.at(mwmId).m_isLoaded)
    {
      hasData = true;
      break;
    }
  }

  ChangeState(hasData ? TransitSchemeState::Enabled : TransitSchemeState::NoData);
}

void TransitReadManager::ClearCache(MwmSet::MwmId const & mwmId)
{
  auto it = m_mwmCache.find(mwmId);
  if (it == m_mwmCache.end())
    return;
  m_cacheSize -= it->second.m_dataSize;
  m_mwmCache.erase(it);
  m_drapeEngine.SafeCall(&df::DrapeEngine::ClearTransitSchemeCache, mwmId);
}

void TransitReadManager::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  MwmSet::MwmId mwmId;
  for (auto const & cacheEntry : m_mwmCache)
  {
    if (cacheEntry.first.IsDeregistered(countryFile))
    {
      mwmId = cacheEntry.first;
      break;
    }
  }
  ClearCache(mwmId);
}

void TransitReadManager::Invalidate()
{
  if (!m_isSchemeMode)
    return;

  if (m_currentModelView.second)
    UpdateViewport(m_currentModelView.first);
}

void TransitReadManager::ShrinkCacheToAllowableSize()
{
  using namespace std::chrono;
  if (m_cacheSize > kMaxTransitCacheSizeBytes)
  {
    std::multimap<time_point<steady_clock>, MwmSet::MwmId> seenTimings;
    for (auto const & entry : m_mwmCache)
      if (entry.second.m_isLoaded && m_lastActiveMwms.count(entry.first) == 0)
        seenTimings.insert(make_pair(entry.second.m_lastActiveTime, entry.first));

    while (m_cacheSize > kMaxTransitCacheSizeBytes && !seenTimings.empty())
    {
      ClearCache(seenTimings.begin()->second);
      seenTimings.erase(seenTimings.begin());
    }
  }
}

bool TransitReadManager::GetTransitDisplayInfo(TransitDisplayInfos & transitDisplayInfos)
{
  unique_lock<mutex> lock(m_mutex);
  auto const groupId = ++m_nextTasksGroupId;
  lock.unlock();

  map<MwmSet::MwmId, unique_ptr<ReadTransitTask>> transitTasks;
  for (auto & mwmTransitPair : transitDisplayInfos)
  {
    auto const & mwmId = mwmTransitPair.first;
    auto task = make_unique<ReadTransitTask>(m_dataSource, m_readFeaturesFn);
    task->Init(groupId, mwmId, std::move(mwmTransitPair.second));
    transitTasks[mwmId] = std::move(task);
  }

  lock.lock();
  m_tasksGroups[groupId] = transitTasks.size();
  lock.unlock();

  for (auto const & task : transitTasks)
    m_threadsPool->PushBack(task.second.get());

  lock.lock();
  m_event.wait(lock, [&]() { return m_tasksGroups[groupId] == 0; });
  m_tasksGroups.erase(groupId);
  lock.unlock();

  bool result = true;
  for (auto const & transitTask : transitTasks)
  {
    if (!transitTask.second->GetSuccess())
    {
      result = false;
      continue;
    }
    transitDisplayInfos[transitTask.first] = transitTask.second->GetTransitInfo();
  }
  return result;
}

void TransitReadManager::OnTaskCompleted(threads::IRoutine * task)
{
  ASSERT(dynamic_cast<ReadTransitTask *>(task) != nullptr, ());
  ReadTransitTask * t = static_cast<ReadTransitTask *>(task);

  lock_guard<mutex> lock(m_mutex);

  if (--m_tasksGroups[t->GetId()] == 0)
    m_event.notify_all();
}

TransitReadManager::TransitSchemeState TransitReadManager::GetState() const
{
  return m_state;
}

void TransitReadManager::SetStateListener(TransitStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void TransitReadManager::ChangeState(TransitSchemeState newState)
{
  if (m_state == newState)
    return;
  m_state = newState;
  if (m_onStateChangedFn)
    m_onStateChangedFn(newState);
}
