#include "testing/testing.hpp"

#include "search/hotels_filter.hpp"

using namespace search::hotels_filter;

namespace
{
UNIT_TEST(HotelsFilter_Identity)
{
  {
    auto const first = And(Or(Eq<Rating>(5.0), Lt<PriceRate>(2)), Ge<Rating>(4.0));
    auto const second = And(Or(Eq<Rating>(5.0), Lt<PriceRate>(2)), Ge<Rating>(4.0));
    TEST(first.get(), ());
    TEST(second.get(), ());
    TEST(first->IdenticalTo(*second), (*first, *second));
  }

  {
    auto const first = And(Gt<Rating>(5.0), Lt<PriceRate>(5));
    auto const second = And(Lt<PriceRate>(5), Gt<Rating>(5.0));
    TEST(first.get(), ());
    TEST(second.get(), ());
    TEST(!first->IdenticalTo(*second), (*first, *second));
  }

  {
    auto const first = Ge<Rating>(1);
    auto const second = Or(Gt<Rating>(1), Eq<Rating>(1));
    TEST(first.get(), ());
    TEST(second.get(), ());
    TEST(!first->IdenticalTo(*second), (*first, *second));
  }
}
}  // namespace
