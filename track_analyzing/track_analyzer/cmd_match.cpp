#include "track_analyzing/serialization.hpp"
#include "track_analyzing/track.hpp"
#include "track_analyzing/track_analyzer/utils.hpp"
#include "track_analyzing/track_matcher.hpp"
#include "track_analyzing/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/zlib.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>

using namespace routing;
using namespace std;
using namespace storage;
using namespace track_analyzing;

namespace
{
using Iter = typename vector<string>::iterator;

void MatchTracks(MwmToTracks const & mwmToTracks, storage::Storage const & storage,
                 NumMwmIds const & numMwmIds, MwmToMatchedTracks & mwmToMatchedTracks)
{
  base::Timer timer;

  uint64_t tracksCount = 0;
  uint64_t pointsCount = 0;
  uint64_t nonMatchedPointsCount = 0;

  auto processMwm = [&](string const & mwmName, UserToTrack const & userToTrack) {
    auto const countryFile = platform::CountryFile(mwmName);
    auto const mwmId = numMwmIds.GetId(countryFile);
    TrackMatcher matcher(storage, mwmId, countryFile);

    auto & userToMatchedTracks = mwmToMatchedTracks[mwmId];

    for (auto const & it : userToTrack)
    {
      string const & user = it.first;
      auto & matchedTracks = userToMatchedTracks[user];
      try
      {
        matcher.MatchTrack(it.second, matchedTracks);
      }
      catch (RootException const & e)
      {
        LOG(LERROR, ("Can't match track for mwm:", mwmName, ", user:", user));
        LOG(LERROR, ("  ", e.what()));
      }

      if (matchedTracks.empty())
        userToMatchedTracks.erase(user);
    }

    if (userToMatchedTracks.empty())
      mwmToMatchedTracks.erase(mwmId);

    tracksCount += matcher.GetTracksCount();
    pointsCount += matcher.GetPointsCount();
    nonMatchedPointsCount += matcher.GetNonMatchedPointsCount();

    LOG(LINFO, (numMwmIds.GetFile(mwmId).GetName(), ", users:", userToTrack.size(), ", tracks:",
                matcher.GetTracksCount(), ", points:", matcher.GetPointsCount(),
                ", non matched points:", matcher.GetNonMatchedPointsCount()));
  };

  ForTracksSortedByMwmName(mwmToTracks, numMwmIds, processMwm);

  LOG(LINFO,
      ("Matching finished, elapsed:", timer.ElapsedSeconds(), "seconds, tracks:", tracksCount,
       ", points:", pointsCount, ", non matched points:", nonMatchedPointsCount));
}
}  // namespace

namespace track_analyzing
{
void CmdMatch(string const & logFile, string const & trackFile, shared_ptr<NumMwmIds> const & numMwmIds, Storage const & storage)
{
  MwmToTracks mwmToTracks;
  ParseTracks(logFile, numMwmIds, mwmToTracks);

  MwmToMatchedTracks mwmToMatchedTracks;
  MatchTracks(mwmToTracks, storage, *numMwmIds, mwmToMatchedTracks);

  FileWriter writer(trackFile, FileWriter::OP_WRITE_TRUNCATE);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Serialize(mwmToMatchedTracks, writer);
  LOG(LINFO, ("Matched tracks were saved to", trackFile));
}

void CmdMatch(string const & logFile, string const & trackFile)
{
  LOG(LINFO, ("Matching", logFile));
  Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);
  CmdMatch(logFile, trackFile, numMwmIds, storage);
}

void UnzipAndMatch(Iter begin, Iter end, string const & trackExt)
{
  Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);
  for (auto it = begin; it != end; ++it)
  {
    auto & file = *it;
    string data;
    try
    {
      auto const r = GetPlatform().GetReader(file);
      r->ReadAsString(data);
    }
    catch (FileReader::ReadException const & e)
    {
      LOG(LWARNING, (e.what()));
      continue;
    }

    using Inflate = coding::ZLib::Inflate;
    Inflate inflate(Inflate::Format::GZip);
    string track;
    inflate(data.data(), data.size(), back_inserter(track));
    base::GetNameWithoutExt(file);
    try
    {
      FileWriter w(file);
      w.Write(track.data(), track.size());
    }
    catch (FileWriter::WriteException const & e)
    {
      LOG(LWARNING, (e.what()));
      continue;
    }

    CmdMatch(file, file + trackExt, numMwmIds, storage);
    FileWriter::DeleteFileX(file);
  }
}

void CmdMatchDir(string const & logDir, string const & trackExt)
{
  Platform::EFileType fileType = Platform::FILE_TYPE_UNKNOWN;
  Platform::EError const result = Platform::GetFileType(logDir, fileType);

  if (result == Platform::ERR_FILE_DOES_NOT_EXIST)
  {
    LOG(LINFO, ("Directory doesn't exist", logDir));
    return;
  }

  if (result != Platform::ERR_OK)
  {
    LOG(LINFO, ("Can't get file type for", logDir));
    return;
  }

  if (fileType != Platform::FILE_TYPE_DIRECTORY)
  {
    LOG(LINFO, (logDir, "is not a directory."));
    return;
  }

  Platform::FilesList filesList;
  Platform::GetFilesRecursively(logDir, filesList);
  if (filesList.empty())
  {
    LOG(LINFO, (logDir, "is empty."));
    return;
  }

  auto const size = filesList.size();
  auto const hardwareConcurrency = static_cast<size_t>(thread::hardware_concurrency());
  CHECK_GREATER(hardwareConcurrency, 0, ("No available threads."));
  LOG(LINFO, ("Number of available threads =", hardwareConcurrency));
  auto const threadsCount = min(size, hardwareConcurrency);
  auto const blockSize = size / threadsCount;
  vector<thread> threads(threadsCount - 1);
  auto begin = filesList.begin();
  for (size_t i = 0; i < threadsCount - 1; ++i)
  {
    auto end = begin + blockSize;
    threads[i] = thread(UnzipAndMatch, begin, end, trackExt);
    begin = end;
  }

  UnzipAndMatch(begin, filesList.end(), trackExt);
  for (auto & t : threads)
    t.join();
}
}  // namespace track_analyzing
