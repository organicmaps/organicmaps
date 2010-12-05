#import  <UIKit/UIKit.h>
#import  "UserLocationController.hpp"

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
	
	m2::PointD m_Pt1, m_Pt2;
}

- (id) initWithCoder: (NSCoder *)coder;

- (void) OnLocation: (m2::PointD const &) mercatorPoint 
			withTimestamp: (NSDate *) timestamp;      
- (void) OnLocationError: (NSString *) errorDescription;

- (void) onResize: (GLint)width withHeight: (GLint)height;
- (void) onPaint;

@end
