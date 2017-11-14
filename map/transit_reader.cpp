#include "map/transit_reader.hpp"

using namespace routing;
using namespace std;

bool TransitReader::Init(MwmSet::MwmId const & mwmId)
{
  m_src = move(GetReader(mwmId));
  if (m_src != nullptr)
    ReadHeader();
  return IsValid();
}

bool TransitReader::IsValid() const
{
  return m_src != nullptr;
}

unique_ptr<TransitReader::TransitReaderSource> TransitReader::GetReader(MwmSet::MwmId const & mwmId)
{
  MwmSet::MwmHandle handle = m_index.GetMwmHandleById(mwmId);
  if (!handle.IsAlive())
  {
    LOG(LWARNING, ("Can't get mwm handle for", mwmId));
    return unique_ptr<TransitReaderSource>();
  }
  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
    return unique_ptr<TransitReaderSource>();
  FilesContainerR::TReader reader = mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG);
  return my::make_unique<TransitReaderSource>(reader);
}

void TransitReader::ReadHeader()
{
  try
  {
    transit::FixedSizeDeserializer<ReaderSource<FilesContainerR::TReader>> fixedSizeDeserializer(*m_src.get());
    fixedSizeDeserializer(m_header);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Error while reading transit header.", e.Msg()));
    throw;
  }
}

void TransitReader::ReadStops(vector<transit::Stop> & stops)
{
  ReadTable(m_header.m_stopsOffset, stops);
}

void TransitReader::ReadShapes(vector<transit::Shape> & shapes)
{
  ReadTable(m_header.m_shapesOffset, shapes);
}

void TransitReader::ReadTransfers(vector<transit::Transfer> & transfers)
{
  ReadTable(m_header.m_transfersOffset, transfers);
}

void TransitReader::ReadLines(vector<transit::Line> & lines)
{
  ReadTable(m_header.m_linesOffset, lines);
}

void TransitReader::ReadNetworks(vector<transit::Network> & networks)
{
  ReadTable(m_header.m_networksOffset, networks);
}

bool ReadTransitTask::Init(uint64_t id, MwmSet::MwmId const & mwmId, unique_ptr<TransitDisplayInfo> && transitInfo)
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
    m_transitInfo = move(transitInfo);
  }
  return m_transitReader.Init(mwmId);
}

void ReadTransitTask::Do()
{
  if (!m_transitReader.IsValid())
    return;

  vector<transit::Stop> stops;
  m_transitReader.ReadStops(stops);
  FillItemsByIdMap(stops, m_transitInfo->m_stops);
  stops.clear();

  for (auto const & stop : m_transitInfo->m_stops)
  {
    if (stop.second.GetFeatureId() != transit::kInvalidFeatureId)
    {
      auto const featureId = FeatureID(m_mwmId, stop.second.GetFeatureId());
      m_transitInfo->m_features[featureId] = {};
    }

    if (stop.second.GetTransferId() != transit::kInvalidTransferId)
      m_transitInfo->m_transfers[stop.second.GetTransferId()] = {};
  }

  vector<transit::Transfer> transfers;
  m_transitReader.ReadTransfers(transfers);
  FillItemsByIdMap(transfers, m_transitInfo->m_transfers);
  transfers.clear();

  vector<transit::Line> lines;
  m_transitReader.ReadLines(lines);
  FillItemsByIdMap(lines, m_transitInfo->m_lines);
  lines.clear();

  vector<transit::Shape> shapes;
  m_transitReader.ReadShapes(shapes);
  FillItemsByIdMap(shapes, m_transitInfo->m_shapes);
  shapes.clear();

  vector<FeatureID> features;
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
  m_id = 0;
  IRoutine::Reset();
}

unique_ptr<TransitDisplayInfo> && ReadTransitTask::GetTransitInfo()
{
  return move(m_transitInfo);
}

TransitReadManager::TransitReadManager(Index & index, TReadFeaturesFn const & readFeaturesFn)
  : m_index(index)
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

  using namespace placeholders;
  uint8_t constexpr kThreadsCount = 2;
  m_threadsPool = my::make_unique<threads::ThreadPool>(
      kThreadsCount, bind(&TransitReadManager::OnTaskCompleted, this, _1));
}

void TransitReadManager::Stop()
{
  if (m_threadsPool != nullptr)
    m_threadsPool->Stop();
  m_threadsPool.reset();
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
    auto task = my::make_unique<ReadTransitTask>(m_index, m_readFeaturesFn);
    if (!task->Init(groupId, mwmId, move(mwmTransitPair.second)))
    {
      LOG(LWARNING, ("Getting transit info failed for", mwmId));
      return false;
    }
    transitTasks[mwmId] = move(task);
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

  for (auto const & transitTask : transitTasks)
    transitDisplayInfos[transitTask.first] = transitTask.second->GetTransitInfo();
  return true;
}

void TransitReadManager::OnTaskCompleted(threads::IRoutine * task)
{
  ASSERT(dynamic_cast<ReadTransitTask *>(task) != nullptr, ());
  ReadTransitTask * t = static_cast<ReadTransitTask *>(task);

  lock_guard<mutex> lock(m_mutex);

  if (--m_tasksGroups[t->GetId()] == 0)
    m_event.notify_all();
}
