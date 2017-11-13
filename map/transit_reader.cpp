#include "map/transit_reader.hpp"

using namespace routing;
using namespace std;

void TransitReader::ReadStops(MwmSet::MwmId const & mwmId, std::vector<transit::Stop> & stops)
{
  ReadTable(mwmId, [](transit::TransitHeader const & header){ return header.m_stopsOffset; }, stops);
}

void TransitReader::ReadShapes(MwmSet::MwmId const & mwmId, std::vector<transit::Shape> & shapes)
{
  ReadTable(mwmId, [](transit::TransitHeader const & header){ return header.m_shapesOffset; }, shapes);
}

void TransitReader::ReadTransfers(MwmSet::MwmId const & mwmId, std::vector<transit::Transfer> & transfers)
{
  ReadTable(mwmId, [](transit::TransitHeader const & header){ return header.m_transfersOffset; }, transfers);
}

void TransitReader::ReadLines(MwmSet::MwmId const & mwmId, std::vector<transit::Line> & lines)
{
  ReadTable(mwmId, [](transit::TransitHeader const & header){ return header.m_linesOffset; }, lines);
}

void TransitReader::ReadNetworks(MwmSet::MwmId const & mwmId, std::vector<transit::Network> & networks)
{
  ReadTable(mwmId, [](transit::TransitHeader const & header){ return header.m_networksOffset; }, networks);
}

void ReadTransitTask::Init(uint64_t id, MwmSet::MwmId const & mwmId, std::unique_ptr<TransitDisplayInfo> && transitInfo)
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
}

void ReadTransitTask::Do()
{
  std::vector<transit::Stop> stops;
  m_transitReader.ReadStops(m_mwmId, stops);
  FillItemsByIdMap(stops, m_transitInfo->m_stops);
  stops.clear();

  std::vector<transit::Line> lines;
  m_transitReader.ReadLines(m_mwmId, lines);
  FillItemsByIdMap(lines, m_transitInfo->m_lines);
  lines.clear();

  std::vector<transit::Shape> shapes;
  m_transitReader.ReadShapes(m_mwmId, shapes);
  FillItemsByIdMap(shapes, m_transitInfo->m_shapes);
  shapes.clear();

  for (auto const & stop : m_transitInfo->m_stops)
  {
    if (stop.second.GetFeatureId() != transit::kInvalidFeatureId)
    {
      auto const featureId = FeatureID(m_mwmId, stop.second.GetFeatureId());
      m_transitInfo->m_features[featureId] = {};
    }
    else
    {
      LOG(LWARNING, ("Invalid feature id for transit stop", stop.first));
    }

    if (stop.second.GetTransferId() != transit::kInvalidTransferId)
      m_transitInfo->m_transfers[stop.second.GetTransferId()] = {};
  }

  std::vector<transit::Transfer> transfers;
  m_transitReader.ReadTransfers(m_mwmId, transfers);
  FillItemsByIdMap(transfers, m_transitInfo->m_transfers);
  transfers.clear();

  std::vector<FeatureID> features;
  for (auto & id : m_transitInfo->m_features)
    features.push_back(id.first);

  m_readFeaturesFn([this](FeatureType const & ft)
  {
    auto & featureInfo = m_transitInfo->m_features[ft.GetID()];
    ft.GetReadableName(featureInfo.m_title);
    featureInfo.m_point = ft.GetCenter();
  }, features);
}

void ReadTransitTask::Reset()
{
  m_transitInfo.reset();
  IRoutine::Reset();
}

std::unique_ptr<TransitDisplayInfo> && ReadTransitTask::GetTransitInfo()
{
  return std::move(m_transitInfo);
}

TransitReadManager::TransitReadManager(Index & index, TReadFeaturesFn const & readFeaturesFn)
  : m_nextTasksGroupId(0)
  , m_transitReader(index)
  , m_readFeaturesFn(readFeaturesFn)
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

  using namespace std::placeholders;
  uint8_t constexpr kThreadsCount = 2;
  m_threadsPool = my::make_unique<threads::ThreadPool>(
      kThreadsCount, std::bind(&TransitReadManager::OnTaskCompleted, this, _1));
}

void TransitReadManager::Stop()
{
  if (m_threadsPool != nullptr)
    m_threadsPool->Stop();
  m_threadsPool.reset();
}

void TransitReadManager::GetTransitDisplayInfo(TransitDisplayInfos & transitDisplayInfos)
{
  auto const groupId = m_nextTasksGroupId++;
  std::map<MwmSet::MwmId, unique_ptr<ReadTransitTask>> transitTasks;
  for (auto & mwmTransitPair : transitDisplayInfos)
  {
    auto task = my::make_unique<ReadTransitTask>(m_transitReader, m_readFeaturesFn);
    task->Init(groupId, mwmTransitPair.first, std::move(mwmTransitPair.second));
    transitTasks[mwmTransitPair.first] = std::move(task);
  }

  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasksGroups[groupId] = transitTasks.size();
    lock.unlock();
    for (auto const &task : transitTasks)
    {
      m_threadsPool->PushBack(task.second.get());
    }
    lock.lock();
    m_event.wait(lock, [&]() { return m_tasksGroups[groupId] == 0; });
    m_tasksGroups.erase(groupId);
  }

  for (auto const & transitTask : transitTasks)
  {
    transitDisplayInfos[transitTask.first] = transitTask.second->GetTransitInfo();
  }
}

void TransitReadManager::OnTaskCompleted(threads::IRoutine * task)
{
  ASSERT(dynamic_cast<ReadTransitTask *>(task) != nullptr, ());
  ReadTransitTask * t = static_cast<ReadTransitTask *>(task);
  std::unique_lock<std::mutex> m_lock(m_mutex);
  if (--m_tasksGroups[t->GetId()] == 0)
    m_event.notify_all();
}
