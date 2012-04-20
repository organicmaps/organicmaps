#include "../../testing/testing.hpp"

#include "../feature_data.hpp"
#include "../feature_visibility.hpp"
#include "../classificator.hpp"
#include "../scales.hpp"


UNIT_TEST(VisibleScales_Smoke)
{
  {
    char const * arr[] = { "place", "city", "capital" };

    feature::TypesHolder types;
    types(classif().GetTypeByPath(vector<string>(arr, arr + 3)));

    pair<int, int> const r = feature::GetDrawableScaleRange(types);
    TEST_NOT_EQUAL(r.first, -1, ());
    TEST_LESS_OR_EQUAL(r.first, r.second, ());

    TEST(my::between_s(r.first, r.second, 10), (r));
    TEST(!my::between_s(r.first, r.second, 1), (r));
    TEST(!my::between_s(r.first, r.second, scales::GetUpperScale()), (r));
  }
}
