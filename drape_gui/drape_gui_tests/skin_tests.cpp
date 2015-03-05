#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../skin.hpp"
#include "../drape_gui.hpp"

UNIT_TEST(ParseDefaultSkinTest)
{
  gui::DrapeGui::Instance().Init([]{ return 2.0; }, [] (ScreenBase const &) { return 6.0f; });
  gui::Skin skin(gui::ResolveGuiSkinFile("default"));
  float width = 600.0f;
  float height = 800.0f;
  skin.Resize(width, height);

  gui::Position compassPos = skin.ResolvePosition(gui::Skin::ElementName::Compass);
  TEST_EQUAL(compassPos.m_anchor, dp::Center, ());
  TEST_ALMOST_EQUAL(compassPos.m_pixelPivot.x, 27.0f * 2.0f, ());
  TEST_ALMOST_EQUAL(compassPos.m_pixelPivot.y, height - 57.0f * 2.0f, ());

  gui::Position rulerPos = skin.ResolvePosition(gui::Skin::ElementName::Ruler);
  TEST_EQUAL(rulerPos.m_anchor, dp::Right, ());
  TEST_ALMOST_EQUAL(rulerPos.m_pixelPivot.x, width - 6.0f * 2.0f, ());
  TEST_ALMOST_EQUAL(rulerPos.m_pixelPivot.y, height - 42.0f * 2.0f, ());

  gui::Position copyRightPos = skin.ResolvePosition(gui::Skin::ElementName::Copyright);
  TEST_EQUAL(copyRightPos.m_anchor, dp::Right, ());
  TEST_ALMOST_EQUAL(copyRightPos.m_pixelPivot.x, width - 6.0f * 2.0f, ());
  TEST_ALMOST_EQUAL(copyRightPos.m_pixelPivot.y, height - 42.0f * 2.0f, ());

  gui::Position countryStatusPos = skin.ResolvePosition(gui::Skin::ElementName::CountryStatus);
  TEST_EQUAL(countryStatusPos.m_anchor, dp::Center, ());
  TEST_ALMOST_EQUAL(countryStatusPos.m_pixelPivot.x, width / 2.0f, ());
  TEST_ALMOST_EQUAL(countryStatusPos.m_pixelPivot.y, height / 2.0f, ());

  {
    width = 800.0f;
    height = 600.0f;
    skin.Resize(width, height);

    gui::Position compassPos = skin.ResolvePosition(gui::Skin::ElementName::Compass);
    TEST_EQUAL(compassPos.m_anchor, dp::Center, ());
    TEST_ALMOST_EQUAL(compassPos.m_pixelPivot.x, 18.0f * 2.0f, ());
    TEST_ALMOST_EQUAL(compassPos.m_pixelPivot.y, height - 11.4f * 2.0f, ());

    gui::Position rulerPos = skin.ResolvePosition(gui::Skin::ElementName::Ruler);
    TEST_EQUAL(rulerPos.m_anchor, dp::Right, ());
    TEST_ALMOST_EQUAL(rulerPos.m_pixelPivot.x, width - 70.4f * 2.0f, ());
    TEST_ALMOST_EQUAL(rulerPos.m_pixelPivot.y, height - 10.5f * 2.0f, ());

    gui::Position copyRightPos = skin.ResolvePosition(gui::Skin::ElementName::Copyright);
    TEST_EQUAL(copyRightPos.m_anchor, dp::Right, ());
    TEST_ALMOST_EQUAL(copyRightPos.m_pixelPivot.x, width - 70.4f * 2.0f, ());
    TEST_ALMOST_EQUAL(copyRightPos.m_pixelPivot.y, height - 10.5f * 2.0f, ());

    gui::Position countryStatusPos = skin.ResolvePosition(gui::Skin::ElementName::CountryStatus);
    TEST_EQUAL(countryStatusPos.m_anchor, dp::Center, ());
    TEST_ALMOST_EQUAL(countryStatusPos.m_pixelPivot.x, width / 2.0f, ());
    TEST_ALMOST_EQUAL(countryStatusPos.m_pixelPivot.y, height / 2.0f, ());
  }
}
