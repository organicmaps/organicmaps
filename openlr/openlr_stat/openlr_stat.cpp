#include "openlr/openlr_decoder.hpp"

#include "routing/road_graph.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "storage/country_parent_getter.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "openlr/decoded_path.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/openlr_model_xml.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_helpers.hpp"

#include "3party/gflags/src/gflags/gflags.h"
#include "3party/pugixml/src/pugixml.hpp"

#include "std/unique_ptr.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

DEFINE_string(input, "", "Path to OpenLR file.");
DEFINE_string(spark_output, "", "Path to output file in spark-oriented format");
DEFINE_string(assessment_output, "", "Path to output file in assessment-tool oriented format");

DEFINE_string(non_matched_ids, "non-matched-ids.txt",
              "Path to a file ids of non-matched segments will be saved to");
DEFINE_string(mwms_path, "", "Path to a folder with mwms.");
DEFINE_string(resources_path, "", "Path to a folder with resources.");
DEFINE_int32(limit, -1, "Max number of segments to handle. -1 for all.");
DEFINE_bool(multipoints_only, false, "Only segments with multiple points to handle.");
DEFINE_int32(num_threads, 1, "Number of threads.");
DEFINE_string(ids_path, "", "Path to a file with segment ids to process.");
DEFINE_string(countries_filename, "",
              "Name of countries file which describes mwm tree. Used to get country specific "
              "routing restrictions.");
DEFINE_int32(algo_version, 0, "Use new decoding algorithm");

using namespace openlr;

namespace
{
int32_t const kMinNumThreads = 1;
int32_t const kMaxNumThreads = 128;
int32_t const kHandleAllSegments = -1;

void LoadDataSources(std::string const & pathToMWMFolder,
                     std::vector<FrozenDataSource> & dataSources)
{
  CHECK(Platform::IsDirectory(pathToMWMFolder), (pathToMWMFolder, "must be a directory."));

  Platform::FilesList files;
  Platform::GetFilesByRegExp(pathToMWMFolder, std::string(".*\\") + DATA_FILE_EXTENSION, files);

  CHECK(!files.empty(), (pathToMWMFolder, "Contains no .mwm files."));

  size_t const numDataSources = dataSources.size();
  std::vector<uint64_t> numCountries(numDataSources);

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
      for (size_t i = 0; i < numDataSources; ++i)
      {
        auto const result = dataSources[i].RegisterMap(localFile);
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

  for (size_t i = 0; i < numDataSources; ++i)
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

bool ValidateMwmPath(char const * flagname, std::string const & value)
{
  if (value.empty())
  {
    printf("--%s should be specified\n", flagname);
    return false;
  }

  return true;
}

bool ValidateVersion(char const * flagname, int32_t value)
{
  if (value == 0)
  {
    printf("--%s should be specified\n", flagname);
    return false;
  }

  if (value != 1 && value != 2)
  {
    printf("--%s should be one of 1 or 2\n", flagname);
    return false;
  }

  return true;
}

bool const g_limitDummy = google::RegisterFlagValidator(&FLAGS_limit, &ValidateLimit);
bool const g_numThreadsDummy =
    google::RegisterFlagValidator(&FLAGS_num_threads, &ValidateNumThreads);
bool const g_mwmsPathDummy = google::RegisterFlagValidator(&FLAGS_mwms_path, &ValidateMwmPath);
bool const g_algoVersion = google::RegisterFlagValidator(&FLAGS_algo_version, &ValidateVersion);

void SaveNonMatchedIds(std::string const & filename, std::vector<DecodedPath> const & paths)
{
  if (filename.empty())
    return;

  std::ofstream ofs(filename);
  for (auto const & p : paths)
  {
    if (p.m_path.empty())
      ofs << p.m_segmentId << endl;
  }
}

std::vector<LinearSegment> LoadSegments(pugi::xml_document & document)
{
  std::vector<LinearSegment> segments;
  if (!ParseOpenlr(document, segments))
  {
    LOG(LERROR, ("Can't parse data."));
    exit(-1);
  }

  OpenLRDecoder::SegmentsFilter filter(FLAGS_ids_path, FLAGS_multipoints_only);
  if (FLAGS_limit != kHandleAllSegments && FLAGS_limit >= 0 &&
      static_cast<size_t>(FLAGS_limit) < segments.size())
  {
    segments.resize(FLAGS_limit);
  }

  my::EraseIf(segments,
              [&filter](LinearSegment const & segment) { return !filter.Matches(segment); });

  std::sort(segments.begin(), segments.end(), my::LessBy(&LinearSegment::m_segmentId));

  return segments;
}

void WriteAssessmentFile(std::string const fileName, pugi::xml_document const & doc,
                         std::vector<DecodedPath> const & paths)
{
  std::unordered_map<uint32_t, pugi::xml_node> xmlSegments;
  for (auto const & xpathNode : doc.select_nodes("//reportSegments"))
  {
    auto const xmlSegment = xpathNode.node();
    xmlSegments[xmlSegment.child("ReportSegmentID").text().as_uint()] = xmlSegment;
  }

  pugi::xml_document result;
  auto segments = result.append_child("Segments");
  auto const dict = doc.select_node(".//Dictionary").node();
  char const xmlns[] = "xmlns";

  // Copy namespaces from <Dictionary> to <Segments>
  for (auto const attr : dict.attributes())
  {
    if (strncmp(xmlns, attr.name(), sizeof(xmlns) - 1) != 0)
      continue;

    // Don't copy default namespace.
    if (strncmp(xmlns, attr.name(), sizeof(xmlns)) == 0)
      continue;

    segments.append_copy(attr);
  }

  for (auto const p : paths)
  {
    auto segment = segments.append_child("Segment");
    {
      auto const xmlSegment = xmlSegments[p.m_segmentId.Get()];
      segment.append_copy(xmlSegment);
    }
    if (!p.m_path.empty())
    {
      auto node = segment.append_child("Route");
      openlr::PathToXML(p.m_path, node);
    }
  }

  result.save_file(fileName.data(), "  " /* indent */);
}
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage("OpenLR stats tool.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_resources_path.empty())
    GetPlatform().SetResourceDir(FLAGS_resources_path);

  classificator::Load();

  auto const numThreads = static_cast<uint32_t>(FLAGS_num_threads);

  std::vector<FrozenDataSource> dataSources(numThreads);

  LoadDataSources(FLAGS_mwms_path, dataSources);

  OpenLRDecoder decoder(dataSources, storage::CountryParentGetter(FLAGS_countries_filename,
                                                              GetPlatform().ResourcesDir()));

  pugi::xml_document document;
  auto const load_result = document.load_file(FLAGS_input.data());
  if (!load_result)
  {
    LOG(LERROR, ("Can't load file", FLAGS_input, ":", load_result.description()));
    exit(-1);
  }

  auto const segments = LoadSegments(document);

  std::vector<DecodedPath> paths(segments.size());
  switch (FLAGS_algo_version)
  {
  case 1: decoder.DecodeV1(segments, numThreads, paths); break;
  case 2: decoder.DecodeV2(segments, numThreads, paths); break;
  default: ASSERT(false, ("There should be no way to fall here"));
  }

  SaveNonMatchedIds(FLAGS_non_matched_ids, paths);
  if (!FLAGS_assessment_output.empty())
    WriteAssessmentFile(FLAGS_assessment_output, document, paths);
  if (!FLAGS_spark_output.empty())
    WriteAsMappingForSpark(FLAGS_spark_output, paths);

  return 0;
}
