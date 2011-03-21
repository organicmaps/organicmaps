#import  <UIKit/UIKit.h>
#import  "UserLocationController.h"

#include "../../geometry/point2d.hpp"
#include "../../yg/texture.hpp"
#include "../../map/framework.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../map/render_queue.hpp"
#include "../../map/navigator.hpp"
#include "../../map/feature_vec_model.hpp"
#include "../../std/shared_ptr.hpp"

@interface MapViewController : UIViewController<UserLocationControllerDelegate>
{
	UserLocationController * m_locationController;

  enum Action
	{
		NOTHING,
		DRAGGING,
		SCALING
	} m_CurrentAction;

	bool m_isDirtyPosition;
	bool m_isSticking;
	size_t m_StickyThreshold;

	m2::PointD m_Pt1, m_Pt2;
}

- (id) initWithCoder: (NSCoder *)coder;

- (void)  OnLocation: (m2::PointD const &) mercatorPoint
withConfidenceRadius: (double) confidenceRadius
			 withTimestamp: (NSDate *) timestamp;
- (void) OnHeading: (CLHeading*) heading;

- (void) OnLocationError: (NSString *) errorDescription;

- (void) onResize: (GLint)width withHeight: (GLint)height;
- (void) onPaint;

// called when app is terminated by system
- (void) OnTerminate;
- (void) OnEnterForeground;
- (void) OnEnterBackground;

- (void) Invalidate;

- (IBAction)OnMyPositionClicked:(id)sender;
- (IBAction)OnSettingsClicked:(id)sender;
- (IBAction)OnGuideClicked:(id)sender;

@end
