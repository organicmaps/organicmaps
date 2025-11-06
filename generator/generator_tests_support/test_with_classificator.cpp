#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "indexer/classificator_loader.hpp"
#include "styles/map_style_manager.hpp"

namespace generator
{
namespace tests_support
{
TestWithClassificator::TestWithClassificator()
{
  MapStyleManager::Instance().SetStyle(MapStyleManager::GetMergedStyleName());
  classificator::Load();
}
}  // namespace tests_support
}  // namespace generator
