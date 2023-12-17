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

  descriptions::LangMeta langsMeta;
  int const sz = FindPageAndFill(path, langsMeta);
  if (sz < 0)
  {
    LOG(LWARNING, ("Page", path, "not found."));
    return;
  }
  else if (sz > 0)
  {
    // Add only new loaded pages (not from cache).
    m_stat.AddSize(sz);
    m_stat.IncPage();
  }

  if (isWikiUrl)
    m_stat.IncNumberWikipediaUrls();
  else
    m_stat.IncNumberWikidataIds();

  m_collection.m_features.push_back({ featureId, std::move(langsMeta) });
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
std::string DescriptionsCollector::MakePathForWikidata(std::string const & wikipediaDir,
                                                       std::string const & wikidataId)
{
  return base::JoinPath(wikipediaDir, "wikidata", wikidataId);
}

// static
std::string DescriptionsCollector::FillStringFromFile(std::string const & fullPath)
{
  std::ifstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(fullPath);
  return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

int DescriptionsCollector::FindPageAndFill(std::string const & path, descriptions::LangMeta & meta)
{
  int size = -1;
  if (!IsValidDir(path))
    return size;

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

    if (size < 0)
      size = 0;

    m_stat.IncCode(code);

    auto res = m_path2Index.try_emplace(base::JoinPath(path, filename), 0);
    if (res.second)
    {
      auto const & filePath = res.first->first;
      auto & strings = m_collection.m_strings;
      res.first->second = strings.size();
      strings.push_back(FillStringFromFile(filePath));

      size_t const sz = strings.back().size();
      CHECK(sz > 0, ("Empty file:", filePath));
      size += sz;
    }

    meta.emplace_back(code, res.first->second);
  }

  return size;
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
    descriptions::Serializer serializer(std::move(collector.m_collection));
    serializer.Serialize(*writer);
    sectionSize = writer->Pos() - sectionSize;
  }

  LOG(LINFO, ("Section", DESCRIPTIONS_FILE_TAG, "is built.",
              "Disk size =", sectionSize, "bytes",
              "Compression ratio =", size / double(sectionSize),
              stat.LangStatisticsToString()));
}
}  // namespace generator
