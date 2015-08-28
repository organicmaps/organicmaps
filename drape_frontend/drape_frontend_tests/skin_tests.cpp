#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "drape_frontend/gui/skin.hpp"

UNIT_TEST(ParseDefaultSkinTest)
{
  gui::Skin skin(gui::ResolveGuiSkinFile("default"), 2.0);
  float width = 600.0f;
  float height = 800.0f;
  skin.Resize(width, height);

  gui::Position compassPos = skin.ResolvePosition(gui::WIDGET_COMPASS);
  TEST_EQUAL(compassPos.m_anchor, dp::Center, ());
  TEST_ALMOST_EQUAL_ULPS(compassPos.m_pixelPivot.x, 35.0f * 2.0f, ());
  TEST_ALMOST_EQUAL_ULPS(compassPos.m_pixelPivot.y, height - 90.0f * 2.0f, ());

  gui::Position rulerPos = skin.ResolvePosition(gui::WIDGET_RULER);
  TEST_EQUAL(rulerPos.m_anchor, dp::Right, ());
  TEST_ALMOST_EQUAL_ULPS(rulerPos.m_pixelPivot.x, width - 70.0f * 2.0f, ());
  TEST_ALMOST_EQUAL_ULPS(rulerPos.m_pixelPivot.y, height - 10.0f * 2.0f, ());

  gui::Position copyRightPos = skin.ResolvePosition(gui::WIDGET_COPYRIGHT);
  TEST_EQUAL(copyRightPos.m_anchor, dp::Center, ());
  TEST_ALMOST_EQUAL_ULPS(copyRightPos.m_pixelPivot.x, width / 2.0f, ());
  TEST_ALMOST_EQUAL_ULPS(copyRightPos.m_pixelPivot.y, 30.0f * 2.0f, ());

  {
    width = 800.0f;
    height = 600.0f;
    skin.Resize(width, height);

    gui::Position compassPos = skin.ResolvePosition(gui::WIDGET_COMPASS);
    TEST_EQUAL(compassPos.m_anchor, dp::Center, ());
    TEST_ALMOST_EQUAL_ULPS(compassPos.m_pixelPivot.x, 35.0f * 2.0f, ());
    TEST_ALMOST_EQUAL_ULPS(compassPos.m_pixelPivot.y, height - 90.0f * 2.0f, ());

    gui::Position rulerPos = skin.ResolvePosition(gui::WIDGET_RULER);
    TEST_EQUAL(rulerPos.m_anchor, dp::Right, ());
    TEST_ALMOST_EQUAL_ULPS(rulerPos.m_pixelPivot.x, width - 70.0f * 2.0f, ());
    TEST_ALMOST_EQUAL_ULPS(rulerPos.m_pixelPivot.y, height - 10.0f * 2.0f, ());

    gui::Position copyRightPos = skin.ResolvePosition(gui::WIDGET_COPYRIGHT);
    TEST_EQUAL(copyRightPos.m_anchor, dp::Center, ());
    TEST_ALMOST_EQUAL_ULPS(copyRightPos.m_pixelPivot.x, width / 2.0f, ());
    TEST_ALMOST_EQUAL_ULPS(copyRightPos.m_pixelPivot.y, 30.0f * 2.0f, ());
  }
}
