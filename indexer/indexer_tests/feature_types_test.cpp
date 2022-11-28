#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

namespace feature_types_test
{

UNIT_TEST(Feature_UselessTypes)
{
  /// @todo Take out TestWithClassificator into some common test support lib.
  classificator::Load();

  auto const & c = classif();

  base::StringIL const arr[] = {
    {"wheelchair", "yes"},
    {"building", "train_station"},
  };

  feature::TypesHolder types;
  for (auto const & t : arr)
    types.Add(c.GetTypeByPath(t));

  types.SortBySpec();
  TEST_EQUAL(*types.cbegin(), c.GetTypeByPath(arr[1]), ());
}

} // namespace feature_types_test
