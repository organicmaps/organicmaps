#import  <UIKit/UIKit.h>

#include "../../geometry/point2d.hpp"
#include "../../yg/texture.hpp"
#include "../../map/framework_factory.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../map/navigator.hpp"
#include "../../map/feature_vec_model.hpp"
#include "../../std/shared_ptr.hpp"

@interface MapViewController : UIViewController
{
  enum Action
	{
		NOTHING,
		DRAGGING,
		SCALING
	} m_CurrentAction;

	bool m_isSticking;
	size_t m_StickyThreshold;
  int m_iconSequenceNumber;

	m2::PointD m_Pt1, m_Pt2;
  
  UIButton * m_myPositionButton;
  NSTimer * m_iconTimer;

  bool m_mapIsVisible;
}

- (void) ZoomToRect: (m2::RectD const &) rect;

- (void) onResize: (GLint)width withHeight: (GLint)height;
- (void) beginPaint;
- (void) doPaint;
- (void) endPaint;

// called when app is terminated by system
- (void) OnTerminate;
- (void) OnEnterForeground;
- (void) OnEnterBackground;

- (IBAction)OnMyPositionClicked:(id)sender;
- (IBAction)OnSettingsClicked:(id)sender;
- (IBAction)OnSearchClicked:(id)sender;

@property (nonatomic, retain) IBOutlet UIButton * m_myPositionButton;

@end
