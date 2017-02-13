#include "openlr/openlr_simple_decoder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "std/cstdint.hpp"
#include "std/cstdio.hpp"
#include "std/vector.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(input, "", "Path to OpenLR file.");
DEFINE_string(output, "output.txt", "Path to output file");
DEFINE_string(mwms_path, "", "Path to a folder with mwms.");
DEFINE_int32(limit, -1, "Max number of segments to handle. -1 for all.");
DEFINE_bool(multipoints_only, false, "Only segments with multiple points to handle.");
DEFINE_int32(num_threads, 1, "Number of threads.");
DEFINE_string(ids_path, "", "Path to a file with segment ids to process.");

using namespace openlr;

namespace
{
const int32_t kMinNumThreads = 1;
const int32_t kMaxNumThreads = 128;

void LoadIndexes(string const & pathToMWMFolder, vector<Index> & indexes)
{
  Platform::FilesList files;
  Platform::GetFilesByRegExp(pathToMWMFolder, string(".*\\") + DATA_FILE_EXTENSION, files);
  for (auto const & fileName : files)
  {

    auto const fullFileName = my::JoinFoldersToPath({pathToMWMFolder}, fileName);
    ModelReaderPtr reader(GetPlatform().GetReader(fullFileName, "f"));
    platform::LocalCountryFile localFile(pathToMWMFolder,
                                         platform::CountryFile(my::FilenameWithoutExt(fileName)),
                                         version::ReadVersionDate(reader));
    LOG(LINFO, ("Found mwm:", fullFileName));
    try
    {
      localFile.SyncWithDisk();
      for (auto & index : indexes)
      {
        CHECK_EQUAL(index.RegisterMap(localFile).second, MwmSet::RegResult::Success,
                    ("Can't register mwm:", localFile));
      }
    }
    catch (RootException const & ex)
    {
      CHECK(false, (ex.Msg(), "Bad mwm file:", localFile));
    }
  }
}

bool ValidateLimit(char const * flagname, int32_t value)
{
  if (value < -1)
  {
    printf("Invalid value for --%s: %d, must be greater or equal to -1\n", flagname,
           static_cast<int>(value));
    return false;
  }

  return true;
}

bool ValidateNumThreads(char const * flagname, int32_t value)
{
  if (value < kMinNumThreads || value > kMaxNumThreads)
  {
    printf("Invalid value for --%s: %d, must be between %d and %d inclusively\n", flagname,
           static_cast<int>(value), static_cast<int>(kMinNumThreads),
           static_cast<int>(kMaxNumThreads));
    return false;
  }

  return true;
}

bool ValidataMwmPath(char const * flagname, string const & value)
{
  if (value.empty())
  {
    printf("--%s should be specified\n", flagname);
    return false;
  }

  return true;
}

bool const g_limitDummy = google::RegisterFlagValidator(&FLAGS_limit, &ValidateLimit);
bool const g_numThreadsDummy =
    google::RegisterFlagValidator(&FLAGS_num_threads, &ValidateNumThreads);
bool const g_mwmsPathDummy = google::RegisterFlagValidator(&FLAGS_mwms_path, &ValidataMwmPath);
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage("OpenLR stats tool.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  classificator::Load();

  vector<Index> indexes(FLAGS_num_threads);
  LoadIndexes(FLAGS_mwms_path, indexes);

  OpenLRSimpleDecoder::SegmentsFilter filter(FLAGS_ids_path, FLAGS_multipoints_only);
  OpenLRSimpleDecoder decoder(FLAGS_input, indexes);
  decoder.Decode(FLAGS_output, FLAGS_limit, filter, FLAGS_num_threads);

  return 0;
}
