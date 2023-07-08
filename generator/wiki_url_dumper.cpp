#include "generator/wiki_url_dumper.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/thread_pool_computational.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <utility>

using namespace feature;

namespace generator
{
WikiUrlDumper::WikiUrlDumper(std::string const & path, std::vector<std::string> const & dataFiles)
  : m_path(path), m_dataFiles(dataFiles) {}

void WikiUrlDumper::Dump(size_t cpuCount) const
{
  CHECK_GREATER(cpuCount, 0, ());

  base::thread_pool::computational::ThreadPool threadPool(cpuCount);
  std::vector<std::future<std::string>> futures;
  futures.reserve(m_dataFiles.size());

  auto const fn = [](std::string const & filename) {
    std::stringstream stringStream;
    DumpOne(filename, stringStream);
    return stringStream.str();
  };

  for (auto const & path : m_dataFiles)
  {
    auto result = threadPool.Submit(fn, path);
    futures.emplace_back(std::move(result));
  }

  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(m_path);
  stream << "MwmPath\tosmId\twikipediaUrl\n";
  for (auto & f : futures)
  {
    auto lines = f.get();
    stream << lines;
  }
}

// static
void WikiUrlDumper::DumpOne(std::string const & path, std::ostream & stream)
{
  auto const & needWikiUrl = ftypes::AttractionsChecker::Instance();
  feature::ForEachFeatureRawFormat(path, [&](FeatureBuilder const & feature, uint64_t /* pos */) {
    if (!needWikiUrl(feature.GetTypesHolder()))
      return;

    auto const wikiUrl = feature.GetMetadata().GetWikiURL();
    if (wikiUrl.empty())
      return;

    stream << path << "\t" << feature.GetMostGenericOsmId() << "\t" <<  wikiUrl << "\n";
  });
}

WikiDataFilter::WikiDataFilter(std::string const & path, std::vector<std::string> const & dataFiles)
  :  m_path(path), m_dataFiles(dataFiles)
{
  std::ifstream stream(m_path);
  if (!stream)
    LOG(LERROR, ("File ", m_path, " not found. Consider skipping Descriptions stage."));
  stream.exceptions(std::fstream::badbit);

  uint64_t id;
  std::string wikidata;
  while (stream)
  {
    stream >> id >> wikidata;
    m_idToWikiData.emplace(base::GeoObjectId(id), wikidata);
  }
}

// static
void WikiDataFilter::FilterOne(std::string const & path, std::map<base::GeoObjectId, std::string> const & idToWikiData,
                               std::ostream & stream)
{
  auto const & needWikiUrl = ftypes::AttractionsChecker::Instance();
  feature::ForEachFeatureRawFormat(path, [&](FeatureBuilder const & feature, uint64_t /* pos */) {
    if (!needWikiUrl(feature.GetTypesHolder()))
      return;

    auto const it = idToWikiData.find(feature.GetMostGenericOsmId());
    if (it == std::end(idToWikiData))
      return;

    stream << it->first.GetEncodedId() << "\t" << it->second << "\n";
  });
}

void WikiDataFilter::Filter(size_t cpuCount)
{
  CHECK_GREATER(cpuCount, 0, ());

  base::thread_pool::computational::ThreadPool threadPool(cpuCount);
  std::vector<std::future<std::string>> futures;
  futures.reserve(m_dataFiles.size());

  auto const fn = [&](std::string const & filename) {
    std::stringstream stringStream;
    FilterOne(filename, m_idToWikiData, stringStream);
    return stringStream.str();
  };

  for (auto const & path : m_dataFiles)
  {
    auto result = threadPool.Submit(fn, path);
    futures.emplace_back(std::move(result));
  }

  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(m_path);
  for (auto & f : futures)
  {
    auto lines = f.get();
    stream << lines;
  }
}
}  // namespace generator
