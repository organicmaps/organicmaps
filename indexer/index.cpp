#include "indexer/index.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"


//////////////////////////////////////////////////////////////////////////////////
// MwmValue implementation
//////////////////////////////////////////////////////////////////////////////////

MwmValue::MwmValue(string const & name)
  : m_cont(GetPlatform().GetReader(name))
{
  m_factory.Load(m_cont);
}

string MwmValue::GetFileName() const
{
  string s = m_cont.GetFileName();
  my::GetNameFromFullPath(s);
  my::GetNameWithoutExt(s);
  return s;
}

//////////////////////////////////////////////////////////////////////////////////
// Index implementation
//////////////////////////////////////////////////////////////////////////////////

bool Index::GetVersion(string const & name, MwmInfo & info) const
{
  MwmValue value(name);

  feature::DataHeader const & h = value.GetHeader();
  if (!h.IsMWMSuitable())
    return false;

  info.m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info.m_minScale = static_cast<uint8_t>(scaleR.first);
  info.m_maxScale = static_cast<uint8_t>(scaleR.second);
  info.m_version = value.GetMwmVersion();

  return true;
}

MwmSet::TMwmValueBasePtr Index::CreateValue(string const & name) const
{
  TMwmValueBasePtr p(new MwmValue(name));
  ASSERT(static_cast<MwmValue &>(*p.get()).GetHeader().IsMWMSuitable(), ());
  return p;
}

Index::Index()
{
}

Index::~Index()
{
  Cleanup();
}

namespace
{
  // Deletes map file denoted by @path and all temporary files related
  // to it.
  void DeleteMapFiles(string const & path, bool deleteReady)
  {
    (void)my::DeleteFileX(path);
    (void)my::DeleteFileX(path + RESUME_FILE_EXTENSION);
    (void)my::DeleteFileX(path + DOWNLOADING_FILE_EXTENSION);

    if (deleteReady)
      (void)my::DeleteFileX(path + READY_FILE_EXTENSION);
  }

  string GetFullPath(string const & fileName)
  {
    return GetPlatform().WritablePathForFile(fileName);
  }

  // Deletes all files related to @fileName and renames
  // @fileName.READY_FILE_EXTENSION to @fileName.
  void ReplaceFileWithReady(string const & fileName)
  {
    string const path = GetFullPath(fileName);
    DeleteMapFiles(path, false /* deleteReady */);
    CHECK(my::RenameFileX(path + READY_FILE_EXTENSION, path), (path));
  }
}

pair<MwmSet::MwmLock, bool> Index::RegisterMap(string const & fileName)
{
  if (GetPlatform().IsFileExistsByFullPath(GetFullPath(fileName + READY_FILE_EXTENSION)))
  {
    pair<MwmSet::MwmLock, UpdateStatus> updateResult = UpdateMap(fileName);
    switch (updateResult.second)
    {
      case UPDATE_STATUS_OK:
        return make_pair(move(updateResult.first), true);
      case UPDATE_STATUS_BAD_FILE:
        return make_pair(move(updateResult.first), false);
      case UPDATE_STATUS_UPDATE_DELAYED:
        // Not dangerous, but it's strange when adding existing maps.
        ASSERT(false, ());
        return make_pair(move(updateResult.first), true);
    }
  }

  pair<MwmSet::MwmLock, bool> result = Register(fileName);
  if (result.second)
    m_observers.ForEach(&Observer::OnMapRegistered, fileName);
  return result;
}

bool Index::DeleteMap(string const & fileName)
{
  {
    lock_guard<mutex> lock(m_lock);

    if (!DeregisterImpl(fileName))
      return false;

    DeleteMapFiles(GetFullPath(fileName), true /* deleteReady */);
  }
  m_observers.ForEach(&Observer::OnMapDeleted, fileName);
  return true;
}

bool Index::AddObserver(Observer & observer) { return m_observers.Add(observer); }

bool Index::RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

pair<MwmSet::MwmLock, Index::UpdateStatus> Index::UpdateMap(string const & fileName)
{
  pair<MwmSet::MwmLock, UpdateStatus> result;
  result.second = UPDATE_STATUS_BAD_FILE;

  {
    lock_guard<mutex> lock(m_lock);

    MwmId const id = GetMwmIdByFileNameImpl(fileName);
    shared_ptr<MwmInfo> info = id.GetInfo();
    if (id.IsAlive() && info->m_lockCount > 0)
    {
      info->SetStatus(MwmInfo::STATUS_PENDING_UPDATE);
      result.first = GetLock(id);
      result.second = UPDATE_STATUS_UPDATE_DELAYED;
    }
    else
    {
      ReplaceFileWithReady(fileName);
      pair<MwmSet::MwmLock, bool> registerResult = RegisterImpl(fileName);
      if (registerResult.second)
      {
        result.first = move(registerResult.first);
        result.second = UPDATE_STATUS_OK;
      }
    }
  }
  if (result.second != UPDATE_STATUS_BAD_FILE)
    m_observers.ForEach(&Observer::OnMapUpdateIsReady, fileName);
  if (result.second == UPDATE_STATUS_OK)
    m_observers.ForEach(&Observer::OnMapUpdated, fileName);
  return result;
}

void Index::OnMwmDeleted(shared_ptr<MwmInfo> const & info)
{
  string const & fileName = info->m_fileName;
  DeleteMapFiles(fileName, true /* deleteReady */);
  m_observers.ForEach(&Observer::OnMapDeleted, fileName);
}

void Index::OnMwmReadyForUpdate(shared_ptr<MwmInfo> const & info)
{
  ClearCache(MwmId(info));
  ReplaceFileWithReady(info->m_fileName);
  info->SetStatus(MwmInfo::STATUS_UP_TO_DATE);
  m_observers.ForEach(&Observer::OnMapUpdated, info->m_fileName);
}

//////////////////////////////////////////////////////////////////////////////////
// Index::FeaturesLoaderGuard implementation
//////////////////////////////////////////////////////////////////////////////////

Index::FeaturesLoaderGuard::FeaturesLoaderGuard(Index const & parent, MwmId id)
    : m_lock(const_cast<Index &>(parent), id),
      /// @note This guard is suitable when mwm is loaded
      m_vector(m_lock.GetValue<MwmValue>()->m_cont, m_lock.GetValue<MwmValue>()->GetHeader())
{
}

string Index::FeaturesLoaderGuard::GetFileName() const
{
  if (!m_lock.IsLocked())
    return string();
  return m_lock.GetValue<MwmValue>()->GetFileName();
}

bool Index::FeaturesLoaderGuard::IsWorld() const
{
  return m_lock.GetValue<MwmValue>()->GetHeader().GetType() == feature::DataHeader::world;
}

void Index::FeaturesLoaderGuard::GetFeature(uint32_t offset, FeatureType & ft)
{
  m_vector.Get(offset, ft);
  ft.SetID(FeatureID(m_lock.GetId(), offset));
}
