#include "qt_tstfrm/gui_test_widget.hpp"

#include "gui/button.hpp"
#include "gui/text_view.hpp"
#include "gui/image_view.hpp"
#include "gui/cached_text_view.hpp"

#include "graphics/display_list.hpp"

#include "map/country_status_display.hpp"
#include "map/framework.hpp"


struct ButtonTest
{
  shared_ptr<gui::Button> m_button;

  void Init(gui::Controller * c)
  {
    gui::Button::Params bp;

    bp.m_depth = graphics::maxDepth - 10;
    bp.m_pivot = m2::PointD(200, 200);
    bp.m_position = graphics::EPosAbove;
    bp.m_text = "TestButton";
    bp.m_minWidth = 200;
    bp.m_minHeight = 40;

    m_button.reset(new gui::Button(bp));

    m_button->setFont(gui::Element::EActive, graphics::FontDesc(16, graphics::Color(255, 255, 255, 255)));
    m_button->setFont(gui::Element::EPressed, graphics::FontDesc(16, graphics::Color(255, 255, 255, 255)));

    m_button->setColor(gui::Element::EActive, graphics::Color(graphics::Color(0, 0, 0, 0.6 * 255)));
    m_button->setColor(gui::Element::EPressed, graphics::Color(graphics::Color(0, 0, 0, 0.4 * 255)));

    c->AddElement(m_button);
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {
  }
};

struct TextViewTest
{
  shared_ptr<gui::TextView> m_textView;

  void Init(gui::Controller * c)
  {
    gui::TextView::Params tp;

    tp.m_pivot = m2::PointD(100, 100);
    tp.m_depth = graphics::maxDepth;
    tp.m_position = graphics::EPosRight;
    tp.m_text = "Simplicity is the ultimate";

    m_textView.reset(new gui::TextView(tp));
    m_textView->setIsVisible(true);

    c->AddElement(m_textView);
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {
  }
};

struct CachedTextViewTest
{
  shared_ptr<gui::CachedTextView> m_cachedTextView;

  void Init(gui::Controller * c)
  {
    gui::CachedTextView::Params ctp;

    ctp.m_pivot = m2::PointD(100, 100);
    ctp.m_depth = graphics::maxDepth;
    ctp.m_position = graphics::EPosAbove;
    ctp.m_text = "(123.15, 783.123)";

    m_cachedTextView.reset(new gui::CachedTextView(ctp));
    m_cachedTextView->setIsVisible(true);

    m_cachedTextView->setFont(gui::Element::EActive,
                              graphics::FontDesc(20, graphics::Color(255, 0, 0, 255), true));

    c->AddElement(m_cachedTextView);
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {
  }
};

struct ImageViewTest
{
  shared_ptr<gui::ImageView> m_imageView;
  m2::PointD m_pivot;

  void Init(gui::Controller * c)
  {
    gui::ImageView::Params ip;

    m_pivot = m2::PointD(100, 100);

    ip.m_depth = graphics::maxDepth;
    ip.m_pivot = m_pivot;
    ip.m_position = graphics::EPosUnder;

    ip.m_image = graphics::Image::Info("arrow.png", graphics::EDensityMDPI);

    m_imageView.reset(new gui::ImageView(ip));

    c->AddElement(m_imageView);

    m_imageView->setIsVisible(true);
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {
    m2::RectD r(m_pivot, m_pivot);

    r.Inflate(2, 2);

    p->drawRectangle(r, graphics::Color(255, 0, 0, 255), graphics::maxDepth);
  }
};

struct CountryStatusDisplayTest
{
  shared_ptr<CountryStatusDisplay> m_countryStatus;
  shared_ptr<Framework> m_framework;
  m2::PointD m_pivot;

  void Init(gui::Controller * c)
  {
    CountryStatusDisplay::Params p(m_framework->GetCountryTree().GetActiveMapLayout());

    m_pivot = m2::PointD(400, 400);

    m_framework.reset(new Framework());

    ///@TODO (UVR)
//    p.m_depth = graphics::maxDepth;
//    p.m_pivot = m_pivot;
//    p.m_position = graphics::EPosAboveLeft;

//    m_countryStatus.reset(new CountryStatusDisplay(p));
//    m_countryStatus->SetCountryIndex(storage::TIndex(1, 1, 1));
//    m_countryStatus->setPivot(m_pivot);
//    c->AddElement(m_countryStatus);
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {
  }
};

//UNIT_TEST_GUI(ButtonTest);
//UNIT_TEST_GUI(TextViewTest);
//UNIT_TEST_GUI(ImageViewTest);
//UNIT_TEST_GUI(CachedTextViewTest);
//UNIT_TEST_GUI(CountryStatusDisplayTest);

