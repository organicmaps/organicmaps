#import  <UIKit/UIKit.h>

#include "../../geometry/point2d.hpp"
#include "../../yg/texture.hpp"
#include "../../map/framework.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../map/render_queue.hpp"
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
  
  UIBarButtonItem * m_myPositionButton;
  NSTimer * m_iconTimer;

  bool m_mapIsVisible;
}

- (void) ZoomToRect: (m2::RectD const &) rect;

- (void) onResize: (GLint)width withHeight: (GLint)height;
- (void) onPaint;

// called when app is terminated by system
- (void) OnTerminate;
- (void) OnEnterForeground;
- (void) OnEnterBackground;

- (IBAction)OnMyPositionClicked:(id)sender;
- (IBAction)OnSettingsClicked:(id)sender;
- (IBAction)OnGuideClicked:(id)sender;

@property (nonatomic, retain) IBOutlet UIBarButtonItem * m_myPositionButton;

@end
