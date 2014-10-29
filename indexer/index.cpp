#include "index.hpp"
#include "data_header.hpp"

#include "../platform/platform.hpp"

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

  void ReplaceFileWithReady(string const & fileName)
  {
    string const path = GetFullPath(fileName);
    DeleteMapFiles(path, false);
    CHECK ( my::RenameFileX(path + READY_FILE_EXTENSION, path), (path) );
  }
}

int Index::AddMap(string const & fileName, m2::RectD & rect)
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
  else
    return Add(fileName, rect);
}

bool Index::DeleteMap(string const & fileName)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  if (!RemoveImpl(fileName))
    return false;

  DeleteMapFiles(GetFullPath(fileName), true);
  return true;
}

int Index::UpdateMap(string const & fileName, m2::RectD & rect)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    m_info[id].m_status = MwmInfo::STATUS_UPDATE;
    return -2;
  }

  ReplaceFileWithReady(fileName);
  return AddImpl(fileName, rect);
}

void Index::UpdateMwmInfo(MwmId id)
{
  switch (m_info[id].m_status)
  {
  case MwmInfo::STATUS_TO_REMOVE:
    if (m_info[id].m_lockCount == 0)
    {
      DeleteMapFiles(m_name[id], true);

      CHECK(RemoveImpl(id), ());
    }
    break;

  case MwmInfo::STATUS_UPDATE:
    if (m_info[id].m_lockCount == 0)
    {
      ClearCache(id);

      ReplaceFileWithReady(m_name[id]);

      m_info[id].m_status = MwmInfo::STATUS_ACTIVE;
    }
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
}
