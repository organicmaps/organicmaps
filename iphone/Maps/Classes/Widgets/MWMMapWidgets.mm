#import "MWMMapWidgets.h"

#include "drape_frontend/gui/skin.hpp"
#include "std/unique_ptr.hpp"

@interface MWMMapWidgets ()

@property (nonatomic) float visualScale;

@end

@implementation MWMMapWidgets
{
  unique_ptr<gui::Skin> m_skin;
}

- (void)setupWidgets:(Framework::DrapeCreationParams &)p
{
  self.visualScale = p.m_visualScale;
  m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("default"), p.m_visualScale));
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach([&p](gui::EWidget widget, gui::Position const & pos)
  {
    p.m_widgetsInitInfo[widget] = pos;
  });
#ifdef DEBUG
  p.m_widgetsInitInfo[gui::WIDGET_SCALE_LABEL] = gui::Position(dp::LeftBottom);
#endif
}

- (void)resize:(CGSize)size
{
  m_skin->Resize(size.width, size.height);
  [self layoutWidgets];
}

- (void)layoutWidgets
{
  if (m_skin == nullptr)
    return;
  gui::TWidgetsLayoutInfo layout;
  if (self.fullScreen)
  {
    layout[gui::WIDGET_RULER] = m_skin->ResolvePosition(gui::WIDGET_RULER).m_pixelPivot;
    layout[gui::WIDGET_COMPASS] = m_skin->ResolvePosition(gui::WIDGET_COMPASS).m_pixelPivot;
  }
  else
  {
    m_skin->ForEach([&layout, &self](gui::EWidget w, gui::Position const & pos)
    {
      m2::PointF pivot = pos.m_pixelPivot;
      switch (w)
      {
        case gui::WIDGET_RULER:
        case gui::WIDGET_COPYRIGHT:
          pivot -= m2::PointF(0.0, self.bottomBound * self.visualScale);
          break;
        case gui::WIDGET_COMPASS:
          pivot += m2::PointF(self.leftBound, -self.bottomBound) * self.visualScale;
          break;
        case gui::WIDGET_SCALE_LABEL:
        case gui::WIDGET_COUNTRY_STATUS:
          break;
      }
      layout[w] = pivot;
    });
  }
  GetFramework().SetWidgetLayout(move(layout));
}

#pragma mark - Properties

- (void)setFullScreen:(BOOL)fullScreen
{
  _fullScreen = fullScreen;
  [self layoutWidgets];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  [self layoutWidgets];
}

- (void)setBottomBound:(CGFloat)bottomBound
{
  _bottomBound = bottomBound;
  [self layoutWidgets];
}

@end
