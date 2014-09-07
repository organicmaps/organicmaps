#include "image_view.hpp"
#include "controller.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/transformations.hpp"

#include "../base/matrix.hpp"


namespace gui
{
  ImageView::Params::Params()
  {}

  ImageView::ImageView(Params const & p)
    : BaseT(p)
  {
    m_image = p.m_image;
  }

  void ImageView::cache()
  {
    graphics::Screen * cs = m_controller->GetCacheScreen();

    m_displayList.reset();
    m_displayList.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(m_displayList.get());

    math::Matrix<double, 3, 3> m =
        math::Shift(
          math::Identity<double, 3>(),
          -(int)m_image.m_size.x / 2, -(int)m_image.m_size.y / 2);

    uint32_t const imageResID = cs->mapInfo(m_image);
    cs->drawImage(m, imageResID, depth());

    cs->setDisplayList(0);
    cs->endFrame();
  }

  void ImageView::purge()
  {
    m_displayList.reset();
  }

  m2::RectD ImageView::GetBoundRect() const
  {
    m2::PointD const sz(m_image.m_size);
    m2::PointD const pt = computeTopLeft(sz, pivot(), position());
    return m2::RectD(pt, pt + sz);
  }

  void ImageView::draw(graphics::OverlayRenderer * r,
                       math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      m2::PointD pt = computeTopLeft(m_image.m_size,
                                     pivot() * m,
                                     position());

      math::Matrix<double, 3, 3> drawM = math::Shift(math::Identity<double, 3>(),
                                                     pt.x + m_image.m_size.x / 2,
                                                     pt.y + m_image.m_size.y / 2);

      r->drawDisplayList(m_displayList.get(), drawM * m);
    }
  }

  void ImageView::setImage(graphics::Image::Info const & info)
  {
    m_image = info;
    setIsDirtyLayout(true);
  }
}
