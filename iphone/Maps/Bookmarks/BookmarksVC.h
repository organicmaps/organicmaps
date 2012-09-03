#import <UIKit/UIKit.h>
#import "LocationManager.h"

@class BalloonView;

@interface BookmarksVC : UITableViewController <LocationObserver>
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;

  LocationManager * m_locationManager;
}

- (id) initWithBalloonView:(BalloonView *)view;

@end
