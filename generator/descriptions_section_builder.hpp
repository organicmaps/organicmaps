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
#include "base/logging.hpp"

#include "defines.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <boost/optional.hpp>

namespace generator_tests
{
class TestDescriptionSectionBuilder;
}  // namespace generator_tests

namespace generator
{
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
  size_t GetSize() const { return m_size; }
  size_t GetPages() const { return m_pages; }
  LangStatistics const & GetLangStatistics() const { return m_langsStat; }

private:
  size_t m_size = 0;
  size_t m_pages = 0;
  LangStatistics m_langsStat = {};
};

class DescriptionsCollectionBuilder
{
public:
  friend class generator_tests::TestDescriptionSectionBuilder;

  DescriptionsCollectionBuilder(std::string const & wikipediaDir, std::string const & mwmFile);

  template <typename Ft, template <typename> class ForEachFromDatAdapter>
  descriptions::DescriptionsCollection MakeDescriptions()
  {
    descriptions::DescriptionsCollection descriptionList;
    auto fn = [&](Ft & f, uint32_t featureId) {
      auto const & wikiChecker = ftypes::WikiChecker::Instance();
      if (!wikiChecker.NeedFeature(f))
        return;

      descriptions::FeatureDescription description;
      auto const wikiUrl = f.GetMetadata().GetWikiURL();
      auto const ret = GetFeatureDescription(wikiUrl, featureId, description);
      CHECK_GREATER_OR_EQUAL(ret, 0, ());
      if (ret == 0)
        return;

      m_stat.AddSize(ret);
      m_stat.IncPage();
      descriptionList.emplace_back(std::move(description));
    };
    ForEachFromDatAdapter<decltype(fn)> adapter;
    adapter(m_mwmFile, std::move(fn));
    return descriptionList;
  }

  DescriptionsCollectionBuilderStat const & GetStat() const { return m_stat; }
  static std::string MakePath(std::string const & wikipediaDir, std::string wikipediaUrl);

private:
  static size_t FillStringFromFile(std::string const & fullPath, int8_t code,
                                   StringUtf8Multilang & str);
  boost::optional<size_t> FindPageAndFill(std::string wikipediaUrl, StringUtf8Multilang & str);
  size_t GetFeatureDescription(std::string const & wikiUrl, uint32_t featureId,
                               descriptions::FeatureDescription & description);

  DescriptionsCollectionBuilderStat m_stat;
  std::string m_wikipediaDir;
  std::string m_mwmFile;
};

template <typename Ft, template <typename> class ForEachFromDatAdapter = ForEachFromDatAdapt>
struct DescriptionsSectionBuilder
{
  static void Build(std::string const & wikipediaDir, std::string const & mwmFile)
  {
    DescriptionsCollectionBuilder descriptionsCollectionBuilder(wikipediaDir, mwmFile);
    auto descriptionList = descriptionsCollectionBuilder.MakeDescriptions<Ft, ForEachFromDatAdapter>();

    auto const & stat = descriptionsCollectionBuilder.GetStat();
    auto const size = stat.GetSize();
    LOG(LINFO, ("Found", stat.GetPages(), "pages for", mwmFile));
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

void BuildDescriptionsSection(std::string const & wikipediaDir, std::string const & mwmFile);
}  // namespace generator
