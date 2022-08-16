#include "generator/descriptions_section_builder.hpp"
#include "generator/utils.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/feature_processor.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/file_writer.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <fstream>
#include <iterator>
#include <sstream>

namespace generator
{
namespace
{
bool IsValidDir(std::string const & path)
{
  return Platform::IsFileExistsByFullPath(path) && Platform::IsDirectory(path);
}
}  // namespace

WikidataHelper::WikidataHelper(std::string const & mwmPath, std::string const & idToWikidataPath)
{
  if (idToWikidataPath.empty())
    return;

  std::string const osmIdsToFeatureIdsPath = mwmPath + OSM2FEATURE_FILE_EXTENSION;
  if (!ParseFeatureIdToOsmIdMapping(osmIdsToFeatureIdsPath, m_featureIdToOsmId))
    LOG(LCRITICAL, ("Mapping parse error for file ", osmIdsToFeatureIdsPath));

  std::ifstream stream(idToWikidataPath);
  if (!stream)
    LOG(LERROR, ("File ", idToWikidataPath, " not found. Consider skipping Descriptions stage."));
  stream.exceptions(std::fstream::badbit);

  uint64_t id;
  std::string wikidataId;
  while (stream)
  {
    stream >> id >> wikidataId;
    strings::Trim(wikidataId);
    m_osmIdToWikidataId.emplace(base::GeoObjectId(id), wikidataId);
  }
}

std::optional<std::string> WikidataHelper::GetWikidataId(uint32_t featureId) const
{
  auto const itFeatureIdToOsmId = m_featureIdToOsmId.find(featureId);
  if (itFeatureIdToOsmId == std::end(m_featureIdToOsmId))
    return {};

  auto const osmId = itFeatureIdToOsmId->second;
  auto const itOsmIdToWikidataId = m_osmIdToWikidataId.find(osmId);
  return itOsmIdToWikidataId == std::end(m_osmIdToWikidataId) ? std::optional<std::string>()
                                                              : itOsmIdToWikidataId->second;
}

std::string DescriptionsCollectionBuilderStat::LangStatisticsToString() const
{
  std::stringstream stream;
  stream << "Language statistics - ";
  if (m_langsStat.empty())
    stream << "(empty)";

  for (size_t code = 0; code < m_langsStat.size(); ++code)
  {
    if (m_langsStat[code] == 0)
      continue;

    stream << StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(code))
           << ":" << m_langsStat[code] << " ";
  }

  return stream.str();
}

void DescriptionsCollector::operator() (FeatureType & ft, uint32_t featureId)
{
  auto const & attractionsChecker = ftypes::AttractionsChecker::Instance();
  if (!attractionsChecker(ft))
    return;

  (*this)(ft.GetMetadata().GetWikiURL(), featureId);
}

void DescriptionsCollector::operator() (std::string const & wikiUrl, uint32_t featureId)
{
  descriptions::FeatureDescription description;
  std::string path;

  // First try to get wikipedia url.
  bool const isWikiUrl = !wikiUrl.empty();
  if (isWikiUrl)
  {
    path = MakePathForWikipedia(m_wikipediaDir, wikiUrl);
  }
  else
  {
    // Second try to get wikidata id.
    auto const wikidataId = m_wikidataHelper.GetWikidataId(featureId);
    if (wikidataId)
      path = MakePathForWikidata(m_wikipediaDir, *wikidataId);
  }

  if (path.empty())
    return;

  auto const ret = GetFeatureDescription(path, featureId, description);
  if (ret == 0)
    return;

  if (isWikiUrl)
    m_stat.IncNumberWikipediaUrls();
  else
    m_stat.IncNumberWikidataIds();

  m_stat.AddSize(ret);
  m_stat.IncPage();

  m_descriptions.push_back(std::move(description));
}

// static
std::string DescriptionsCollector::MakePathForWikipedia(std::string const & wikipediaDir, std::string wikipediaUrl)
{
  strings::Trim(wikipediaUrl);
  strings::ReplaceFirst(wikipediaUrl, "http://", "");
  strings::ReplaceFirst(wikipediaUrl, "https://", "");
  if (strings::EndsWith(wikipediaUrl, "/"))
    wikipediaUrl.pop_back();

  return base::JoinPath(wikipediaDir, wikipediaUrl);
}

// static
std::string DescriptionsCollector::MakePathForWikidata(std::string const & wikipediaDir, std::string wikidataId)
{
  return base::JoinPath(wikipediaDir, "wikidata", wikidataId);
}

// static
size_t DescriptionsCollector::FillStringFromFile(std::string const & fullPath, int8_t code, StringUtf8Multilang & str)
{
  std::fstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(fullPath);
  std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  auto const contentSize = content.size();
  if (contentSize != 0)
    str.AddString(code, std::move(content));

  return contentSize;
}

size_t DescriptionsCollector::FindPageAndFill(std::string const & path, StringUtf8Multilang & str)
{
  if (!IsValidDir(path))
  {
    LOG(LWARNING, ("Directory", path, "not found."));
    return 0;
  }

  size_t size = 0;
  Platform::FilesList filelist;
  Platform::GetFilesByExt(path, ".html", filelist);
  for (auto const & filename : filelist)
  {
    auto const lang = base::FilenameWithoutExt(filename);
    auto const code = StringUtf8Multilang::GetLangIndex(lang);
    if (code == StringUtf8Multilang::kUnsupportedLanguageCode)
    {
      LOG(LWARNING, (lang, "is an unsupported language."));
      continue;
    }

    m_stat.IncCode(code);
    auto const fullPath = base::JoinPath(path, filename);
    size += FillStringFromFile(fullPath, code, str);
  }

  return size;
}

size_t DescriptionsCollector::GetFeatureDescription(std::string const & path, uint32_t featureId,
                                                    descriptions::FeatureDescription & description)
{
  if (path.empty())
    return 0;

  StringUtf8Multilang string;
  size_t const sz = FindPageAndFill(path, string);
  if (sz == 0)
    return 0;

  description = descriptions::FeatureDescription(featureId, std::move(string));
  return sz;
}

// static
void DescriptionsSectionBuilder::CollectAndBuild(std::string const & wikipediaDir, std::string const & mwmFile,
                                                 std::string const & idToWikidataPath)
{
  DescriptionsCollector collector(wikipediaDir, mwmFile, idToWikidataPath);
  feature::ForEachFeature(mwmFile, collector);
  BuildSection(mwmFile, collector);
}

// static
void DescriptionsSectionBuilder::BuildSection(std::string const & mwmFile, DescriptionsCollector & collector)
{
  auto const & stat = collector.m_stat;
  size_t const size = stat.GetTotalSize();
  LOG(LINFO, ("Wiki descriptions for", mwmFile,
              "Wikipedia urls =", stat.GetNumberOfWikipediaUrls(),
              "Wikidata ids =", stat.GetNumberOfWikidataIds(),
              "Total number of pages =", stat.GetNumberOfPages(),
              "Total size of added pages (before writing to section) =", size, "bytes"));
  if (size == 0)
  {
    LOG(LWARNING, ("Section", DESCRIPTIONS_FILE_TAG, "was not created."));
    return;
  }

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);

  /// @todo Should we override FilesContainerWriter::GetSize() to return local size (not container size)?
  uint64_t sectionSize;
  {
    auto writer = cont.GetWriter(DESCRIPTIONS_FILE_TAG);
    sectionSize = writer->Pos();
    descriptions::Serializer serializer(std::move(collector.m_descriptions));
    serializer.Serialize(*writer);
    sectionSize = writer->Pos() - sectionSize;
  }

  LOG(LINFO, ("Section", DESCRIPTIONS_FILE_TAG, "is built."
              "Disk size =", sectionSize, "bytes",
              "Compression ratio =", size / double(sectionSize),
              stat.LangStatisticsToString()));
}
}  // namespace generator
