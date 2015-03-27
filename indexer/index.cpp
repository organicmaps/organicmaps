#include "index.hpp"
#include "data_header.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../coding/file_name_utils.hpp"
#include "../coding/internal/file_data.hpp"


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
// Index::MwmLock implementation
//////////////////////////////////////////////////////////////////////////////////

string Index::MwmLock::GetFileName() const
{
  MwmValue * p = GetValue();
  return (p ? p->GetFileName() : string());
}

//////////////////////////////////////////////////////////////////////////////////
// Index implementation
//////////////////////////////////////////////////////////////////////////////////

int Index::GetInfo(string const & name, MwmInfo & info) const
{
  MwmValue value(name);

  feature::DataHeader const & h = value.GetHeader();
  if (h.IsMWMSuitable())
  {
    info.m_limitRect = h.GetBounds();

    pair<int, int> const scaleR = h.GetScaleRange();
    info.m_minScale = static_cast<uint8_t>(scaleR.first);
    info.m_maxScale = static_cast<uint8_t>(scaleR.second);

    return h.GetVersion();
  }
  else
    return -1;
}

MwmValue * Index::CreateValue(string const & name) const
{
  MwmValue * p = new MwmValue(name);
  ASSERT(p->GetHeader().IsMWMSuitable(), ());
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

int Index::RegisterMap(string const & fileName, m2::RectD & rect)
{
  if (GetPlatform().IsFileExistsByFullPath(GetFullPath(fileName + READY_FILE_EXTENSION)))
  {
    int const ret = UpdateMap(fileName, rect);
    switch (ret)
    {
    case -1:
      return -1;
    case -2:
      // Not dangerous, but it's strange when adding existing maps.
      ASSERT(false, ());
      return feature::DataHeader::v3;
    default:
      return ret;
    }
  }

  int rv = Register(fileName, rect);
  if (rv >= 0)
    m_observers.ForEach(&Observer::OnMapRegistered, fileName);
  return rv;
}

bool Index::DeleteMap(string const & fileName)
{
  {
    lock_guard<mutex> lock(m_lock);

    if (!DeregisterImpl(fileName))
      return false;

    DeleteMapFiles(GetFullPath(fileName), true);
  }
  m_observers.ForEach(&Observer::OnMapDeleted, fileName);
  return true;
}

bool Index::AddObserver(Observer & observer)
{
  return m_observers.Add(observer);
}

bool Index::RemoveObserver(Observer & observer)
{
  return m_observers.Remove(observer);
}

int Index::UpdateMap(string const & fileName, m2::RectD & rect)
{
  int rv = -1;
  {
    lock_guard<mutex> lock(m_lock);

    MwmId const id = GetIdByName(fileName);
    if (id != INVALID_MWM_ID && m_info[id].m_lockCount > 0)
    {
      m_info[id].SetStatus(MwmInfo::STATUS_PENDING_UPDATE);
      rv = -2;
    } else {
      ReplaceFileWithReady(fileName);
      rv = RegisterImpl(fileName, rect);
    }
  }
  if (rv != -1)
    m_observers.ForEach(&Observer::OnMapUpdateIsReady, fileName);
  if (rv >= 0)
    m_observers.ForEach(&Observer::OnMapUpdated, fileName);
  return rv;
}

void Index::UpdateMwmInfo(MwmId id)
{
  switch (m_info[id].GetStatus())
  {
    case MwmInfo::STATUS_MARKED_TO_DEREGISTER:
      if (m_info[id].m_lockCount == 0)
      {
        string const name = m_name[id];
        DeleteMapFiles(name, true);
        CHECK(DeregisterImpl(id), ());
        m_observers.ForEach(&Observer::OnMapDeleted, name);
      }
      break;

    case MwmInfo::STATUS_PENDING_UPDATE:
      if (m_info[id].m_lockCount == 0)
      {
        ClearCache(id);
        ReplaceFileWithReady(m_name[id]);
        m_info[id].SetStatus(MwmInfo::STATUS_UP_TO_DATE);
        m_observers.ForEach(&Observer::OnMapUpdated, m_name[id]);
      }
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////////
// Index::FeaturesLoaderGuard implementation
//////////////////////////////////////////////////////////////////////////////////

Index::FeaturesLoaderGuard::FeaturesLoaderGuard(Index const & parent, MwmId id)
  : m_lock(parent, id),
    /// @note This guard is suitable when mwm is loaded
    m_vector(m_lock.GetValue()->m_cont, m_lock.GetValue()->GetHeader())
{
}

bool Index::FeaturesLoaderGuard::IsWorld() const
{
  return (m_lock.GetValue()->GetHeader().GetType() == feature::DataHeader::world);
}

void Index::FeaturesLoaderGuard::GetFeature(uint32_t offset, FeatureType & ft)
{
  m_vector.Get(offset, ft);
  ft.SetID(FeatureID(m_lock.GetID(), offset));
}
