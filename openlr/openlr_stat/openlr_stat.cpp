#include "openlr/openlr_simple_decoder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

DEFINE_string(input, "", "Path to OpenLR file.");
DEFINE_string(output, "output.txt", "Path to output file");
DEFINE_string(non_matched_ids, "non-matched-ids.txt",
              "Path to a file ids of non-matched segments will be saved to");
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

void LoadIndexes(std::string const & pathToMWMFolder, std::vector<Index> & indexes)
{
  CHECK(Platform::IsDirectory(pathToMWMFolder), (pathToMWMFolder, "must be a directory."));

  Platform::FilesList files;
  Platform::GetFilesByRegExp(pathToMWMFolder, std::string(".*\\") + DATA_FILE_EXTENSION, files);

  CHECK(!files.empty(), (pathToMWMFolder, "Contains no .mwm files."));

  size_t const numIndexes = indexes.size();
  std::vector<uint64_t> numCountries(numIndexes);

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
      for (size_t i = 0; i < numIndexes; ++i)
      {
        auto const result = indexes[i].RegisterMap(localFile);
        CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register mwm:", localFile));

        auto const & info = result.first.GetInfo();
        if (info && info->GetType() == MwmInfo::COUNTRY)
          ++numCountries[i];
      }
    }
    catch (RootException const & ex)
    {
      CHECK(false, (ex.Msg(), "Bad mwm file:", localFile));
    }
  }

  for (size_t i = 0; i < numIndexes; ++i)
  {
    if (numCountries[i] == 0)
      LOG(LWARNING, ("No countries for thread", i));
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

bool ValidataMwmPath(char const * flagname, std::string const & value)
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

  auto const numThreads = static_cast<uint32_t>(FLAGS_num_threads);

  std::vector<Index> indexes(numThreads);
  LoadIndexes(FLAGS_mwms_path, indexes);

  OpenLRSimpleDecoder::SegmentsFilter filter(FLAGS_ids_path, FLAGS_multipoints_only);
  OpenLRSimpleDecoder decoder(FLAGS_input, indexes);
  decoder.Decode(FLAGS_output, FLAGS_non_matched_ids, FLAGS_limit, filter, numThreads);

  return 0;
}
