#include "map/gps_track.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

namespace
{

size_t const kGpsCollectionMaxItemCount = 100000;
size_t const kGpsFileMaxItemCount = 100000;
hours const kDefaultDuration = hours(24);

} // namespace

size_t const GpsTrack::kInvalidId = GpsTrackCollection::kInvalidId;

GpsTrack::GpsTrack(string const & filePath)
  : m_filePath(filePath)
  , m_duration(kDefaultDuration)
{
}

void GpsTrack::AddPoint(location::GpsTrackInfo const & info)
{
  lock_guard<mutex> lg(m_guard);

  LazyInitFile();

  // Write point to the file.
  // If file exception happens, then drop file.
  if (m_file)
  {
    try
    {
      size_t evictedId;
      m_file->Append(info, evictedId);
    }
    catch (RootException & e)
    {
      LOG(LINFO, ("GpsTrackFile.Append has caused exception:", e.Msg()));
      m_file.reset();
    }
  }

  if (!m_collection)
    return;

  // Write point to the collection
  pair<size_t, size_t> evictedIds;
  size_t const addedId = m_collection->Add(info, evictedIds);
  if (addedId == GpsTrackCollection::kInvalidId)
    return; // nothing was added

  if (!m_callback)
    return;

  vector<pair<size_t, location::GpsTrackInfo>> toAdd;
  toAdd.emplace_back(addedId, info);

  m_callback(move(toAdd), move(evictedIds));
}

void GpsTrack::AddPoints(vector<location::GpsTrackInfo> const & points)
{
  if (points.empty())
    return;

  lock_guard<mutex> lg(m_guard);

  LazyInitFile();

  // Write point to the file.
  // If file exception happens, then drop file.
  if (m_file)
  {
    try
    {
      for (auto const & point : points)
      {
        size_t evictedId;
        m_file->Append(point, evictedId);
      }
    }
    catch (RootException & e)
    {
      LOG(LINFO, ("GpsTrackFile.Append has caused exception:", e.Msg()));
      m_file.reset();
    }
  }

  if (!m_collection)
    return;

  // Add points
  pair<size_t, size_t> evictedIds;
  pair<size_t, size_t> const addedIds = m_collection->Add(points, evictedIds);
  if (addedIds.first == GpsTrackCollection::kInvalidId)
    return; // nothing was added

  if (!m_callback)
    return;

  size_t const addedCount = addedIds.second - addedIds.first + 1;
  ASSERT_GREATER_OR_EQUAL(m_collection->GetSize(), addedCount, ());

  // Not all points from infos could be added to collection due to timestamp consequence restriction.
  // Get added points from collection.
  vector<pair<size_t, location::GpsTrackInfo>> toAdd;
  toAdd.reserve(addedCount);
  m_collection->ForEach([&toAdd](location::GpsTrackInfo const & point, size_t id)->bool
  {
    toAdd.emplace_back(id, point);
    return true;
  }, m_collection->GetSize() - addedCount);
  ASSERT_EQUAL(toAdd.size(), addedCount, ());

  m_callback(move(toAdd), evictedIds);
}

void GpsTrack::Clear()
{
  lock_guard<mutex> lg(m_guard);

  LazyInitFile();

  if (m_file)
  {
    try
    {
      m_file->Clear();
    }
    catch (RootException & e)
    {
      LOG(LINFO, ("GpsTrackFile.Clear has caused exception:", e.Msg()));
      m_file.reset();
    }
  }

  if (!m_collection)
    return;

  auto const evictedIds = m_collection->Clear();
  if (evictedIds.first == GpsTrackCollection::kInvalidId)
    return; // nothing was removed

  if (!m_callback)
    return;

  m_callback(vector<pair<size_t, location::GpsTrackInfo>>(), evictedIds);
}

void GpsTrack::SetDuration(hours duration)
{
  lock_guard<mutex> lg(m_guard);

  m_duration = duration;

  if (!m_collection)
    return;

  auto const evictedIds = m_collection->SetDuration(duration);
  if (evictedIds.first == GpsTrackCollection::kInvalidId)
    return; // nothing was removed

  if (!m_callback)
    return;

  m_callback(vector<pair<size_t, location::GpsTrackInfo>>(), evictedIds);
}

hours GpsTrack::GetDuration() const
{
  lock_guard<mutex> lg(m_guard);

  return m_duration;
}

void GpsTrack::SetCallback(TGpsTrackDiffCallback callback)
{
  lock_guard<mutex> lg(m_guard);

  m_callback = callback;

  if (!callback)
    return;

  LazyInitCollection();

  SendInitialSnapshot();
}

void GpsTrack::LazyInitFile()
{
  // Must be called under m_guard lock

  if (m_file)
    return;

  m_file = make_unique<GpsTrackFile>();

  // Open or create gps track file
  try
  {
    if (!m_file->Open(m_filePath, kGpsFileMaxItemCount))
    {
      if (!m_file->Create(m_filePath, kGpsFileMaxItemCount))
      {
        LOG(LINFO, ("Cannot open or create GpsTrackFile:", m_filePath));
        m_file.reset();
      }
      else
      {
        LOG(LINFO, ("GpsTrackFile has been created:", m_filePath));
      }
    }
  }
  catch (GpsTrackFile::CorruptedFileException &)
  {
    // File has been corrupted.
    // Drop any data from the file.
    try
    {
      LOG(LINFO, ("File is corrupted, create new:", m_filePath));
      if (!m_file->Create(m_filePath, kGpsFileMaxItemCount))
      {
        LOG(LINFO, ("Cannot create GpsTrackFile:", m_filePath));
        m_file.reset();
      }
      else
      {
        LOG(LINFO, ("GpsTrackFile has been created:", m_filePath));
      }
    }
    catch (RootException & e)
    {
      LOG(LINFO, ("GpsTrackFile.Create has caused exception:", e.Msg()));
      m_file.reset();
    }
  }
  catch (RootException & e)
  {
    LOG(LINFO, ("GpsTrackFile has caused exception:", e.Msg()));
    m_file.reset();
  }
}

void GpsTrack::LazyInitCollection()
{
  // Must be called under m_guard lock

  if (m_collection)
    return;

  m_collection = make_unique<GpsTrackCollection>(kGpsCollectionMaxItemCount, m_duration);

  LazyInitFile();

  if (!m_file)
    return;

  try
  {
    // Read points from file to the collection
    m_file->ForEach([this](location::GpsTrackInfo const & info, size_t /* id */)->bool
    {
      pair<size_t, size_t> evictedIds;
      m_collection->Add(info, evictedIds);
      return true;
    });
  }
  catch (GpsTrackFile::CorruptedFileException &)
  {
    LOG(LINFO, ("GpsTrackFile is corrupted, clear it:", m_filePath));
    m_collection->Clear();
    try
    {
      m_file->Clear();
    }
    catch (RootException & e)
    {
      LOG(LINFO, ("GpsTrackFile.Clear caused exception:", e.Msg()));
      m_file.reset();
    }
  }
}

void GpsTrack::SendInitialSnapshot()
{
  // Must be called under m_guard lock

  if (!m_callback)
    return;

  // Get points from collection to send them to the callback
  vector<pair<size_t, location::GpsTrackInfo>> toAdd;
  toAdd.reserve(m_collection->GetSize());
  m_collection->ForEach([&toAdd](location::GpsTrackInfo const & info, size_t id)->bool
  {
    toAdd.emplace_back(id, info);
    return true;
  });

  if (toAdd.empty())
    return; // nothing to send

  m_callback(move(toAdd), make_pair(kInvalidId, kInvalidId));
}

GpsTrack & GetDefaultGpsTrack()
{
  static GpsTrack instance(my::JoinFoldersToPath(GetPlatform().WritableDir(), GPS_TRACK_FILENAME));
  return instance;
}
