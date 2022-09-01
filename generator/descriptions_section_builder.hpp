#pragma once
#include "descriptions/serdes.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <array>
#include <string>
#include <unordered_map>

class FeatureType;

namespace generator
{
class WikidataHelper
{
public:
  WikidataHelper(std::string const & mwmPath, std::string const & idToWikidataPath);

  std::optional<std::string> GetWikidataId(uint32_t featureId) const;

private:
  std::unordered_map<uint32_t, base::GeoObjectId> m_featureIdToOsmId;
  std::unordered_map<base::GeoObjectId, std::string> m_osmIdToWikidataId;
};

class DescriptionsCollectionBuilderStat
{
public:
  using LangStatistics = std::array<size_t, StringUtf8Multilang::kMaxSupportedLanguages>;

  DescriptionsCollectionBuilderStat()
  {
    CHECK_EQUAL(m_langsStat.size(), StringUtf8Multilang::kMaxSupportedLanguages , ());
  }

  std::string LangStatisticsToString() const;
  void IncCode(int8_t code)
  {
    CHECK(code < StringUtf8Multilang::kMaxSupportedLanguages, (code));
    CHECK(code >= 0, (code));

    m_langsStat[static_cast<size_t>(code)] += 1;
  }
  void AddSize(size_t size) { m_size += size; }
  void IncPage() { ++m_pages; }
  void IncNumberWikipediaUrls() { ++m_numberWikipediaUrls; }
  void IncNumberWikidataIds() { ++m_numberWikidataIds; }
  size_t GetTotalSize() const { return m_size; }
  size_t GetNumberOfPages() const { return m_pages; }
  size_t GetNumberOfWikipediaUrls() const { return m_numberWikipediaUrls; }
  size_t GetNumberOfWikidataIds() const { return m_numberWikidataIds; }
  LangStatistics const & GetLangStatistics() const { return m_langsStat; }

private:
  size_t m_size = 0;
  size_t m_pages = 0;
  size_t m_numberWikipediaUrls = 0;
  size_t m_numberWikidataIds = 0;
  LangStatistics m_langsStat = {};
};

class DescriptionsCollector
{
public:
  DescriptionsCollector(std::string const & wikipediaDir, std::string const & mwmFile,
                        std::string const & idToWikidataPath = {})
  : m_wikidataHelper(mwmFile, idToWikidataPath), m_wikipediaDir(wikipediaDir), m_mwmFile(mwmFile)
  {}

  void operator() (FeatureType & ft, uint32_t featureId);
  void operator() (std::string const & wikiUrl, uint32_t featureId);

  static std::string MakePathForWikipedia(std::string const & wikipediaDir, std::string wikipediaUrl);
  static std::string MakePathForWikidata(std::string const & wikipediaDir, std::string const & wikidataId);

  static std::string FillStringFromFile(std::string const & fullPath);

  /// @return -1 If page not found. 0 if page from cache. Size > 0 if page was loaded from disk.
  int FindPageAndFill(std::string const & wikipediaUrl, descriptions::LangMeta & meta);

public:
  DescriptionsCollectionBuilderStat m_stat;
  descriptions::DescriptionsCollection m_collection;

private:
  std::unordered_map<std::string, descriptions::StringIndex> m_path2Index;

  WikidataHelper m_wikidataHelper;
  std::string m_wikipediaDir;
  std::string m_mwmFile;
};

struct DescriptionsSectionBuilder
{
  /// @param[in]  idToWikidataPath  Maybe empty.
  static void CollectAndBuild(std::string const & wikipediaDir, std::string const & mwmFile,
                              std::string const & idToWikidataPath);

  static void BuildSection(std::string const & mwmFile, DescriptionsCollector & collector);
};

}  // namespace generator
