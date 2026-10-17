#include "testing/testing.hpp"

#include "map/user_mark.hpp"

namespace user_mark_tests
{
UNIT_TEST(ContactMark_RenderingProperties)
{
  ContactMarkPoint mark(m2::PointD::Zero());
  mark.SetName("Cody Ellis");
  mark.SetAddress("123 Main Street");
  TEST(mark.IsSymbolSelectable(), ());
  TEST_EQUAL(mark.GetAddress(), "123 Main Street", ());

  TEST_EQUAL(mark.GetMinZoom(), 16, ());
  TEST(!mark.GetDepthTestEnabled(), ());
  TEST(mark.IsMarkAboveText(), ());

  auto const titles = mark.GetTitleDecl();
  TEST(titles != nullptr, ());
  TEST_EQUAL(titles->size(), 1, ());
  TEST_EQUAL(titles->front().m_primaryText, "Cody Ellis", ());
}
}  // namespace user_mark_tests
