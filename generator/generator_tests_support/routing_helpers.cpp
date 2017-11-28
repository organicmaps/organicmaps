#include "generator/generator_tests_support/routing_helpers.hpp"

#include "testing/testing.hpp"

#include "generator/gen_mwm_info.hpp"
#include "base/osm_id.hpp"

#include "coding/file_writer.hpp"

#include "base/osm_id.hpp"
#include "base/string_utils.hpp"

#include <utility>

namespace generator
{
void ReEncodeOsmIdsToFeatureIdsMapping(std::string const & mappingContent, std::string const & outputFilePath)
{
  strings::SimpleTokenizer lineIter(mappingContent, "\n\r" /* line delimiters */);

  gen::Accumulator<std::pair<osm::Id, uint32_t>> osmIdsToFeatureIds;
  for (; lineIter; ++lineIter)
  {
    strings::SimpleTokenizer idIter(*lineIter, ", \t" /* id delimiters */);
    uint64_t osmId = 0;
    TEST(idIter, ());
    TEST(strings::to_uint64(*idIter, osmId), ("Cannot convert to uint64_t:", *idIter));
    TEST(idIter, ("Wrong feature ids to osm ids mapping."));
    ++idIter;

    uint32_t featureId = 0;
    TEST(idIter, ());
    TEST(strings::to_uint(*idIter, featureId), ("Cannot convert to uint:", *idIter));
    osmIdsToFeatureIds.Add(std::make_pair(osm::Id::Way(osmId), featureId));
    ++idIter;
    TEST(!idIter, ());
  }

  FileWriter osm2ftWriter(outputFilePath);
  osmIdsToFeatureIds.Flush(osm2ftWriter);
}
}  // namespace generator
