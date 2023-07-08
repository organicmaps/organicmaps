#include "generator/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "indexer/feature_processor.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/reader_streambuf.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <exception>
#include <iostream>
#include <vector>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

namespace generator
{
void ErrorHandler(int signum)
{
  // Avoid recursive calls.
  std::signal(signum, SIG_DFL);
  // If there was an exception, then we will print the message.
  try
  {
    if (auto const eptr = std::current_exception())
      std::rethrow_exception(eptr);
  }
  catch (RootException const & e)
  {
    std::cerr << "Core exception: " << e.Msg() << "\n";
  }
  catch (std::exception const & e)
  {
    std::cerr << "Std exception: " << e.what() << "\n";
  }
  catch (...)
  {
    std::cerr << "Unknown exception.\n";
  }

  // Print stack stack.
  std::cerr << boost::stacktrace::stacktrace();
  // We raise the signal SIGABRT, so that there would be an opportunity to make a core dump.
  std::raise(SIGABRT);
}

// SingleMwmDataSource -----------------------------------------------------------------------------
SingleMwmDataSource::SingleMwmDataSource(std::string const & mwmPath)
{
  m_countryFile = platform::LocalCountryFile::MakeTemporary(mwmPath);
  m_countryFile.SyncWithDisk();
  CHECK(m_countryFile.OnDisk(MapFileType::Map),
        ("No correct mwm corresponding to local country file:", m_countryFile, ". Path:", mwmPath));

  auto const result = m_dataSource.Register(m_countryFile);
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());
  CHECK(result.first.IsAlive(), ());
  m_mwmId = result.first;
}

FeatureGetter::FeatureGetter(std::string const & countryFullPath)
  : m_mwm(countryFullPath)
  , m_guard(std::make_unique<FeaturesLoaderGuard>(m_mwm.GetDataSource(), m_mwm.GetMwmId()))
{
}

std::unique_ptr<FeatureType> FeatureGetter::GetFeatureByIndex(uint32_t index) const
{
  return m_guard->GetFeatureByIndex(index);
}

bool ParseFeatureIdToOsmIdMapping(std::string const & path,
                                  std::unordered_map<uint32_t, base::GeoObjectId> & mapping)
{
  return ForEachOsmId2FeatureId(
      path, [&](auto const & compositeId, auto featureId) {
        CHECK(mapping.emplace(featureId, compositeId.m_mainId).second,
              ("Several osm ids for feature", featureId, "in file", path));
      });
}

bool ParseFeatureIdToTestIdMapping(std::string const & path,
                                   std::unordered_map<uint32_t, uint64_t> & mapping)
{
  bool success = true;
  feature::ForEachFeature(path, [&](FeatureType & feature, uint32_t fid)
  {
    auto const testIdStr = feature.GetMetadata(feature::Metadata::FMD_TEST_ID);
    uint64_t testId;
    if (!strings::to_uint(testIdStr, testId))
    {
      LOG(LERROR, ("Can't parse test id from:", testIdStr, "for the feature", fid));
      success = false;
      return;
    }
    mapping.emplace(fid, testId);
  });
  return success;
}

search::CBV GetLocalities(std::string const & dataPath)
{
  FrozenDataSource dataSource;
  auto const result = dataSource.Register(platform::LocalCountryFile::MakeTemporary(dataPath));
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register", dataPath));

  search::MwmContext context(dataSource.GetMwmHandleById(result.first));
  base::Cancellable const cancellable;
  return search::CategoriesCache(search::LocalitiesSource{}, cancellable).Get(context);
}

bool MapcssRule::Matches(std::vector<OsmElement::Tag> const & tags) const
{
  for (auto const & tag : m_tags)
  {
    if (!base::AnyOf(tags, [&](auto const & t) { return t == tag; }))
      return false;
  }

  for (auto const & key : m_mandatoryKeys)
  {
    if (!base::AnyOf(tags, [&](auto const & t) { return t.m_key == key && t.m_value != "no"; }))
      return false;
  }

  for (auto const & key : m_forbiddenKeys)
  {
    if (!base::AllOf(tags, [&](auto const & t) { return t.m_key != key || t.m_value == "no"; }))
      return false;
  }

  return true;
}

// The data/mapcss-mapping.csv format is described inside the file itself.
MapcssRules ParseMapCSS(std::unique_ptr<Reader> reader)
{
  ReaderStreamBuf buffer(std::move(reader));
  std::istream data(&buffer);
  data.exceptions(std::fstream::badbit);

  MapcssRules rules;

  auto const processShort = [&rules](std::string const & typeString)
  {
    auto typeTokens = strings::Tokenize<std::string>(typeString, "|");
    CHECK(typeTokens.size() == 2, (typeString));
    MapcssRule rule;
    rule.m_tags = {{typeTokens[0], typeTokens[1]}};
    rules.emplace_back(std::move(typeTokens), std::move(rule));
  };

  auto const processFull = [&rules](std::string const & typeString,
                                    std::string const & selectorsString)
  {
    ASSERT(!typeString.empty(), ());
    ASSERT(!selectorsString.empty(), ());

    auto const typeTokens = strings::Tokenize<std::string>(typeString, "|");
    strings::Tokenize(selectorsString, ",", [&typeTokens, &rules](std::string_view selector)
    {
      ASSERT_EQUAL(selector.front(), '[', ());
      ASSERT_EQUAL(selector.back(), ']', ());

      MapcssRule rule;
      strings::Tokenize(selector, "[]", [&rule](std::string_view kv)
      {
        auto const tag = strings::Tokenize(kv, "=");
        if (tag.size() == 1)
        {
          auto const forbidden = (tag[0][0] == '!');

          std::string v(tag[0]);
          strings::Trim(v, "?!");
          if (forbidden)
            rule.m_forbiddenKeys.push_back(std::move(v));
          else
            rule.m_mandatoryKeys.push_back(std::move(v));
        }
        else
        {
          ASSERT_EQUAL(tag.size(), 2, (tag));
          rule.m_tags.emplace_back(tag[0], tag[1]);
        }
      });

      rules.emplace_back(typeTokens, std::move(rule));
    });
  };

  std::string line;
  while (std::getline(data, line))
  {
    if (line.empty() || line.front() == '#')
      continue;

    std::vector<std::string> fields;
    strings::ParseCSVRow(line, ';', fields);
    CHECK(fields.size() == 3 || fields.size() == 7, (fields.size(), fields, line));
    // Short format without replacement.
    if (fields.size() == 3 && fields[2].empty())
      processShort(fields[0]);

    // Full format, not obsolete.
    if (fields.size() == 7 && fields[2] != "x")
      processFull(fields[0], fields[1]);
  }

  return rules;
}

std::ofstream OfstreamWithExceptions(std::string const & name)
{
  std::ofstream f;
  f.exceptions(std::ios::failbit | std::ios::badbit);
  f.open(name);
  return f;
}
}  // namespace generator
