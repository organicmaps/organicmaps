#include "../../qt_tstfrm/gui_test_widget.hpp"

#include "../balloon.hpp"
#include "../button.hpp"
#include "../text_view.hpp"
#include "../image_view.hpp"

struct BalloonTest
{
  shared_ptr<gui::Balloon> m_balloon;

  bool m_flag;

  void Init(gui::Controller * c)
  {
    m_flag = false;
    gui::Balloon::Params bp;

    bp.m_depth = graphics::maxDepth - 20;
    bp.m_pivot = m2::PointD(200, 200);
    bp.m_position = graphics::EPosAbove;

    bp.m_textMarginLeft = 10;
    bp.m_textMarginRight = 10;
    bp.m_textMarginTop = 5;
    bp.m_textMarginBottom = 5;

    bp.m_imageMarginLeft = 0;
    bp.m_imageMarginRight = 10;
    bp.m_imageMarginTop = 5;
    bp.m_imageMarginBottom = 5;

    bp.m_text = "Active";
    bp.m_image = graphics::Image::Info("testing/images/arrow.png");

    m_balloon.reset(new gui::Balloon(bp));
    m_balloon->setIsVisible(true);

    m_balloon->setOnClickListener(bind(&BalloonTest::OnClick, this, _1));

    c->AddElement(m_balloon);
  }

  void OnClick(gui::Element * e)
  {
    gui::Balloon * b = static_cast<gui::Balloon *>(e);

    SetText(b);

    m_flag = !m_flag;
  }

  void SetText(gui::Balloon * b)
  {
    if (m_flag)
      b->setText("Arrow");
    else
      b->setText("Cinema");
  }

  void DoDraw(shared_ptr<graphics::Screen> const & p)
  {}
};

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

    ip.m_image = graphics::Image::Info("testing/images/arrow.png");

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

UNIT_TEST_GUI(BalloonTest);
UNIT_TEST_GUI(ButtonTest);
UNIT_TEST_GUI(TextViewTest);
UNIT_TEST_GUI(ImageViewTest);

