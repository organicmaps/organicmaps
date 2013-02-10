#import <UIKit/UIKit.h>
#import "LocationManager.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"

#include "Framework.h"

@class BalloonView;

@interface MapViewController : UIViewController <LocationObserver, UIGestureRecognizerDelegate>
{
  BalloonView * m_balloonView;

  CGPoint p1;
  CGPoint p2;
  BOOL startedScaling;
  CFAbsoluteTime lastRotateTime;
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

@property (nonatomic, retain) IBOutlet UIButton * m_myPositionButton;

@end
