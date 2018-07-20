#include "track_analyzing/exceptions.hpp"
#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include <string>

using namespace std;
using namespace track_analyzing;

namespace
{
#define DEFINE_string_ext(name, value, description)                           \
  DEFINE_string(name, value, description);                                    \
                                                                              \
  string const & Checked_##name()                                             \
  {                                                                           \
    if (FLAGS_##name.empty())                                                 \
      MYTHROW(MessageException, (string("Specify the argument --") + #name)); \
                                                                              \
    return FLAGS_##name;                                                      \
  }

DEFINE_string_ext(cmd, "",
                  "command:\n"
                  "match - based on raw logs gathers points to tracks and matches them to "
                  "features. To use the tool raw logs should be taken from \"trafin\" production "
                  "project in gz files and extracted.\n"
                  "unmatched_tracks - based on raw logs gathers points to tracks\n"
                  "and save tracks to csv. Track points save as lat, log, timestamp in seconds\n"
                  "tracks - prints track statistics\n"
                  "track - prints info about single track\n"
                  "cpptrack - prints track coords to insert them to cpp code\n"
                  "table - prints csv table based on matched tracks\n"
                  "gpx - convert raw logs into gpx files\n");
DEFINE_string_ext(in, "", "input log file name");
DEFINE_string(out, "", "output track file name");
DEFINE_string_ext(output_dir, "", "output dir for gpx files");
DEFINE_string_ext(mwm, "", "short mwm name");
DEFINE_string_ext(user, "", "user id");
DEFINE_int32(track, -1, "track index");

DEFINE_string(track_extension, ".track", "track files extension");
DEFINE_bool(no_world_logs, false, "don't print world summary logs");
DEFINE_bool(no_mwm_logs, false, "don't print logs per mwm");
DEFINE_bool(no_track_logs, false, "don't print logs per track");

DEFINE_uint64(min_duration, 5 * 60, "minimum track duration in seconds");
DEFINE_double(min_length, 1000.0, "minimum track length in meters");
DEFINE_double(min_speed, 15.0, "minimum track average speed in km/hour");
DEFINE_double(max_speed, 110.0, "maximum track average speed in km/hour");
DEFINE_bool(ignore_traffic, true, "ignore tracks with traffic data");

size_t Checked_track()
{
  if (FLAGS_track < 0)
    MYTHROW(MessageException, ("Specify the --track key"));

  return static_cast<size_t>(FLAGS_track);
}

StringFilter MakeFilter(string const & filter)
{
  return [&](string const & value) {
    if (filter.empty())
      return false;
    return value != filter;
  };
}
}  // namespace

namespace track_analyzing
{
// Print the specified track in C++ form that you can copy paste to C++ source for debugging.
void CmdCppTrack(string const & trackFile, string const & mwmName, string const & user,
                 size_t trackIdx);
// Match raw gps logs to tracks.
void CmdMatch(string const & logFile, string const & trackFile);
// Parse |logFile| and save tracks (mwm name, aloha id, lats, lons, timestamps in seconds in csv).
void CmdUnmatchedTracks(string const & logFile, string const & trackFileCsv);
// Print aggregated tracks to csv table.
void CmdTagsTable(string const & filepath, string const & trackExtension,
                  StringFilter mwmIsFiltered, StringFilter userFilter);
// Print track information.
void CmdTrack(string const & trackFile, string const & mwmName, string const & user,
              size_t trackIdx);
// Print tracks statistics.
void CmdTracks(string const & filepath, string const & trackExtension, StringFilter mwmFilter,
               StringFilter userFilter, TrackFilter const & filter, bool noTrackLogs,
               bool noMwmLogs, bool noWorldLogs);

void CmdGPX(string const & logFile, string const & outputFilesDir, string const & userID);
}  // namespace track_analyzing

int main(int argc, char ** argv)
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  string const & cmd = Checked_cmd();

  classificator::Load();
  classif().SortClassificator();

  try
  {
    if (cmd == "match")
    {
      string const & logFile = Checked_in();
      CmdMatch(logFile, FLAGS_out.empty() ? logFile + ".track" : FLAGS_out);
    }
    else if (cmd == "unmatched_tracks")
    {
      string const & logFile = Checked_in();
      CmdUnmatchedTracks(logFile, FLAGS_out.empty() ? logFile + ".track.csv" : FLAGS_out);
    }
    else if (cmd == "tracks")
    {
      TrackFilter const filter(FLAGS_min_duration, FLAGS_min_length, FLAGS_min_speed,
                               FLAGS_max_speed, FLAGS_ignore_traffic);
      CmdTracks(Checked_in(), FLAGS_track_extension, MakeFilter(FLAGS_mwm), MakeFilter(FLAGS_user),
                filter, FLAGS_no_track_logs, FLAGS_no_mwm_logs, FLAGS_no_world_logs);
    }
    else if (cmd == "track")
    {
      CmdTrack(Checked_in(), Checked_mwm(), Checked_user(), Checked_track());
    }
    else if (cmd == "cpptrack")
    {
      CmdCppTrack(Checked_in(), Checked_mwm(), Checked_user(), Checked_track());
    }
    else if (cmd == "table")
    {
      CmdTagsTable(Checked_in(), FLAGS_track_extension, MakeFilter(FLAGS_mwm),
                   MakeFilter(FLAGS_user));
    }
    else if (cmd == "gpx")
    {
      CmdGPX(Checked_in(), Checked_output_dir(), FLAGS_user);
    }
    else
    {
      LOG(LWARNING, ("Unknown command", FLAGS_cmd));
      return 1;
    }
  }
  catch (MessageException const & e)
  {
    LOG(LWARNING, (e.Msg()));
    return 1;
  }
  catch (RootException const & e)
  {
    LOG(LERROR, (e.what()));
    return 1;
  }

  return 0;
}
