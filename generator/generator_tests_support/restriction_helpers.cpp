#include "generator/generator_tests_support/restriction_helpers.hpp"

#include "testing/testing.hpp"

#include "generator/gen_mwm_info.hpp"

#include "coding/file_writer.hpp"

#include "base/string_utils.hpp"

#include "std/utility.hpp"

namespace generator
{
void ReEncodeOsmIdsToFeatureIdsMapping(string const & mappingContent, string const & outputFilePath)
{
  strings::SimpleTokenizer lineIter(mappingContent, "\n\r" /* line delimiters */);

  gen::Accumulator<pair<uint64_t, uint32_t>> osmIdsToFeatureIds;
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
    osmIdsToFeatureIds.Add(make_pair(osmId, featureId));
    ++idIter;
    TEST(!idIter, ());
  }

  FileWriter osm2ftWriter(outputFilePath);
  osmIdsToFeatureIds.Flush(osm2ftWriter);
}
}  // namespace generator
