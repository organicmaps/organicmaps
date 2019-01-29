#pragma once

#include "descriptions/serdes.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include <boost/optional.hpp>

namespace generator_tests
{
class TestDescriptionSectionBuilder;
}  // namespace generator_tests

namespace generator
{
class WikidataHelper
{
public:
  WikidataHelper() = default;
  explicit WikidataHelper(std::string const & mwmPath, std::string const & id2wikidataPath);

  boost::optional<std::string> GetWikidataId(uint32_t featureId) const;

private:
  std::string m_mwmPath;
  std::string m_id2wikidataPath;
  std::map<uint32_t, base::GeoObjectId> m_featureIdToOsmId;
  std::map<base::GeoObjectId, std::string> m_osmIdToFeatureId;
};

template <class T>
struct ForEachFromDatAdapt
{
  void operator()(std::string const & str, T && fn) const
  {
    feature::ForEachFromDat(str, std::forward<T>(fn));
  }
};

class DescriptionsCollectionBuilderStat
{
public:
  using LangStatistics = std::array<size_t, StringUtf8Multilang::kMaxSupportedLanguages>;

  DescriptionsCollectionBuilderStat()
  {
    CHECK_EQUAL(m_langsStat.size(),
                static_cast<size_t>(StringUtf8Multilang::kMaxSupportedLanguages) , ());
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
  size_t GetSize() const { return m_size; }
  size_t GetPages() const { return m_pages; }
  size_t GetNumberWikipediaUrls() const { return m_numberWikipediaUrls; }
  size_t GetNumberWikidataIds() const { return m_numberWikidataIds; }
  LangStatistics const & GetLangStatistics() const { return m_langsStat; }

private:
  size_t m_size = 0;
  size_t m_pages = 0;
  size_t m_numberWikipediaUrls = 0;
  size_t m_numberWikidataIds = 0;
  LangStatistics m_langsStat = {};
};

class DescriptionsCollectionBuilder
{
public:
  friend class generator_tests::TestDescriptionSectionBuilder;

  DescriptionsCollectionBuilder(std::string const & wikipediaDir, std::string const & mwmFile,
                                std::string const & id2wikidataPath);
  DescriptionsCollectionBuilder(std::string const & wikipediaDir, std::string const & mwmFile);

  template <typename Ft, template <typename> class ForEachFromDatAdapter>
  descriptions::DescriptionsCollection MakeDescriptions()
  {
    descriptions::DescriptionsCollection descriptionList;
    auto fn = [&](Ft & f, uint32_t featureId) {
      auto const & wikiChecker = ftypes::WikiChecker::Instance();
      if (!wikiChecker.NeedFeature(f))
        return;

      std::function<void()> incSource = []() {};
      descriptions::FeatureDescription description;
      std::string path;
      // We first try to get wikipedia url.
      auto const wikiUrl = f.GetMetadata().GetWikiURL();
      if (!wikiUrl.empty())
      {
        path = MakePathForWikipedia(m_wikipediaDir, wikiUrl);
        incSource = std::bind(&DescriptionsCollectionBuilderStat::IncNumberWikipediaUrls, std::ref(m_stat));
      }
      else
      {
        // We second try to get wikidata id.
        auto const wikidataId = m_wikidataHelper.GetWikidataId(featureId);
        if (wikidataId)
        {
          path = MakePathForWikidata(m_wikipediaDir, *wikidataId);
          incSource = std::bind(&DescriptionsCollectionBuilderStat::IncNumberWikidataIds, std::ref(m_stat));
        }
      }

      if (path.empty())
        return;

      auto const ret = GetFeatureDescription(path, featureId, description);
      if (ret == 0)
        return;

      incSource();
      m_stat.AddSize(ret);
      m_stat.IncPage();
      descriptionList.emplace_back(std::move(description));
    };
    ForEachFromDatAdapter<decltype(fn)> adapter;
    adapter(m_mwmFile, std::move(fn));
    return descriptionList;
  }

  DescriptionsCollectionBuilderStat const & GetStat() const { return m_stat; }
  static std::string MakePathForWikipedia(std::string const & wikipediaDir, std::string wikipediaUrl);
  static std::string MakePathForWikidata(std::string const & wikipediaDir, std::string wikidataId);

private:
  static size_t FillStringFromFile(std::string const & fullPath, int8_t code,
                                   StringUtf8Multilang & str);
  boost::optional<size_t> FindPageAndFill(std::string wikipediaUrl, StringUtf8Multilang & str);
  size_t GetFeatureDescription(std::string const & wikiUrl, uint32_t featureId,
                               descriptions::FeatureDescription & description);

  DescriptionsCollectionBuilderStat m_stat;
  WikidataHelper m_wikidataHelper;
  std::string m_wikipediaDir;
  std::string m_mwmFile;
};

template <typename Ft, template <typename> class ForEachFromDatAdapter = ForEachFromDatAdapt>
struct DescriptionsSectionBuilder
{
  static void Build(std::string const & wikipediaDir, std::string const & mwmFile,
                    std::string const & id2wikidataPath)
  {
    DescriptionsCollectionBuilder descriptionsCollectionBuilder(wikipediaDir, mwmFile, id2wikidataPath);
    Build(mwmFile, descriptionsCollectionBuilder);
  }

  static void Build(std::string const & wikipediaDir, std::string const & mwmFile)
  {
    DescriptionsCollectionBuilder descriptionsCollectionBuilder(wikipediaDir, mwmFile);
    Build(mwmFile, descriptionsCollectionBuilder);
  }

private:
  static void Build(std::string const & mwmFile, DescriptionsCollectionBuilder & builder)
  {
    auto descriptionList = builder.MakeDescriptions<Ft, ForEachFromDatAdapter>();
    auto const & stat = builder.GetStat();
    auto const size = stat.GetSize();
    LOG(LINFO, ("Added", stat.GetNumberWikipediaUrls(), "pages form wikipedia urls for", mwmFile));
    LOG(LINFO, ("Added", stat.GetNumberWikidataIds(), "pages form wikidata ids for", mwmFile));
    LOG(LINFO, ("Added", stat.GetPages(), "pages for", mwmFile));
    LOG(LINFO, ("Total size of added pages (before writing to section):", size));
    CHECK_GREATER_OR_EQUAL(size, 0, ());
    if (size == 0)
    {
      LOG(LWARNING, ("Section", DESCRIPTIONS_FILE_TAG, "was not created for", mwmFile));
      return;
    }

    FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
    FileWriter writer = cont.GetWriter(DESCRIPTIONS_FILE_TAG);
    descriptions::Serializer serializer(std::move(descriptionList));
    serializer.Serialize(writer);

    LOG(LINFO, ("Section", DESCRIPTIONS_FILE_TAG, "is built for", mwmFile));
    LOG(LINFO, (stat.LangStatisticsToString()));
  }
};

void BuildDescriptionsSection(std::string const & wikipediaDir, std::string const & mwmFile,
                              std::string const & id2wikidataPath);

void BuildDescriptionsSection(std::string const & wikipediaDir, std::string const & mwmFile);
}  // namespace generator
