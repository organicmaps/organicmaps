#import <UIKit/UIKit.h>
#import "LocationManager.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"

#include "Framework.h"

@class BalloonView;

@interface MapViewController : UIViewController <LocationObserver, UIAlertViewDelegate, UIPopoverControllerDelegate>
{
  enum Action
	{
		NOTHING,
		DRAGGING,
		SCALING
	} m_CurrentAction;

	bool m_isSticking;
	size_t m_StickyThreshold;
	m2::PointD m_Pt1, m_Pt2;
  
  BalloonView * m_balloonView;
  /// Temporary solution to improve long touch detection.
  m2::PointD m_touchDownPoint;
  UIPopoverController * popover;
}

- (void) SetupMeasurementSystem;

// called when app is terminated by system
- (void) OnTerminate;
- (void) OnEnterForeground;
- (void) OnEnterBackground;

- (IBAction)OnMyPositionClicked:(id)sender;
- (IBAction)OnSettingsClicked:(id)sender;
- (IBAction)OnSearchClicked:(id)sender;
- (IBAction)OnBookmarksClicked:(id)sender;

- (void)showSearchResultAsBookmarkAtMercatorPoint:(m2::PointD const &)pt withInfo:(Framework::AddressInfo const &)info;
- (void)showBalloonWithCategoryIndex:(int)index andBookmarkIndex:(int)bmIndex;
- (void)showBalloonWithText:(NSString *)text andGlobalPoint:(m2::PointD) point;

- (void)dismissPopoverAndSaveBookmark:(BOOL)save;

@property (nonatomic, retain) IBOutlet UIButton * m_myPositionButton;

@end
