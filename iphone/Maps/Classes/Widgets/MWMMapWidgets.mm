#import "MWMMapWidgets.h"
#import "EAGLView.h"
#import "MapViewController.h"

@interface MWMMapWidgets ()

@property(nonatomic) float visualScale;
@property(nonatomic) CGRect availableArea;

@end

@implementation MWMMapWidgets
{
  unique_ptr<gui::Skin> m_skin;
}

+ (MWMMapWidgets *)widgetsManager
{
  return ((EAGLView *)[MapViewController sharedController].view).widgetsManager;
}

- (void)setupWidgets:(Framework::DrapeCreationParams &)p
{
  self.visualScale = p.m_visualScale;
  m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("default"), p.m_visualScale));
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach(
      [&p](gui::EWidget widget, gui::Position const & pos) { p.m_widgetsInitInfo[widget] = pos; });
  p.m_widgetsInitInfo[gui::WIDGET_SCALE_FPS_LABEL] = gui::Position(dp::LeftBottom);
}

- (void)resize:(CGSize)size
{
  m_skin->Resize(size.width, size.height);
  dispatch_async(dispatch_get_main_queue(), ^{
    [self updateAvailableArea:self.availableArea];
  });
}

- (void)updateAvailableArea:(CGRect)frame
{
  if (CGRectEqualToRect(self.availableArea, frame))
    return;
  self.availableArea = frame;
  if (m_skin == nullptr)
    return;
  gui::TWidgetsLayoutInfo layout;
  auto const vs = self.visualScale;
  auto const viewHeight = [MapViewController sharedController].view.height;
  auto const viewWidth = [MapViewController sharedController].view.width;
  auto const rulerOffset =
      m2::PointF(frame.origin.x * vs, (frame.origin.y + frame.size.height - viewHeight) * vs);
  auto const compassOffset =
      m2::PointF((frame.origin.x + frame.size.width - viewWidth) * vs, frame.origin.y * vs);
  m_skin->ForEach([&](gui::EWidget w, gui::Position const & pos) {
    m2::PointF pivot = pos.m_pixelPivot;
    switch (w)
    {
    case gui::WIDGET_RULER:
    case gui::WIDGET_WATERMARK:
    case gui::WIDGET_COPYRIGHT: pivot += rulerOffset; break;
    case gui::WIDGET_COMPASS: pivot += compassOffset; break;
    case gui::WIDGET_SCALE_FPS_LABEL:
    case gui::WIDGET_CHOOSE_POSITION_MARK: break;
    }
    layout[w] = pivot;
  });
  GetFramework().SetWidgetLayout(move(layout));
}

@end
