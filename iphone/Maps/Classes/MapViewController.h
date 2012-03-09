#import <UIKit/UIKit.h>
#import "LocationManager.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"

@interface MapViewController : UIViewController <LocationObserver>
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
  
  NSTimer * m_iconTimer;
}

- (void) ZoomToRect: (m2::RectD const &) rect;

- (void) SetupMeasurementSystem;

// called when app is terminated by system
- (void) OnTerminate;
- (void) OnEnterForeground;
- (void) OnEnterBackground;

- (IBAction)OnMyPositionClicked:(id)sender;
- (IBAction)OnSettingsClicked:(id)sender;
- (IBAction)OnSearchClicked:(id)sender;

@property (nonatomic, retain) IBOutlet UIButton * m_myPositionButton;
@property (nonatomic, retain) IBOutlet UIButton * m_searchButton;
@property (nonatomic, retain) IBOutlet UIButton * m_downloadButton;

@end
