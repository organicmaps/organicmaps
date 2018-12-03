#include "generator/wiki_url_dumper.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <cstdint>
#include <fstream>

namespace generator
{
WikiUrlDumper::WikiUrlDumper(std::string const & path, std::vector<std::string> const & dataFiles)
  : m_path(path), m_dataFiles(dataFiles) {}

void WikiUrlDumper::Dump() const
{
  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(m_path);
  stream << "MwmPath\tosmId\twikipediaUrl\n";
  for (auto const & path : m_dataFiles)
    DumpOne(path, stream);
}

// static
void WikiUrlDumper::DumpOne(std::string const & path, std::ostream & stream)
{
  auto const & needWikiUrl = ftypes::WikiChecker::Instance();
  feature::ForEachFromDatRawFormat(path, [&](FeatureBuilder1 const & feature, uint64_t /* pos */) {
    if (!needWikiUrl(feature.GetTypesHolder()))
      return;

    auto const wikiUrl = feature.GetMetadata().GetWikiURL();
    if (wikiUrl.empty())
      return;

    stream << path << "\t" << feature.GetMostGenericOsmId() << "\t" <<  wikiUrl << "\n";
  });
}
}  // namespace generator
