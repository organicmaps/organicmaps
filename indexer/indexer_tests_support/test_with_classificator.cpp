#include "indexer/indexer_tests_support/test_with_classificator.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style.hpp"
#include "indexer/map_style_reader.hpp"

namespace indexer
{
namespace tests_support
{
TestWithClassificator::TestWithClassificator()
{
  GetStyleReader().SetCurrentStyle(MapStyleMerged);
  classificator::Load();
}
}  // namespace tests_support
}  // namespace indexer
