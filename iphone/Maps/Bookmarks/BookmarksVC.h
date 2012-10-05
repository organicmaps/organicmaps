#import <UIKit/UIKit.h>
#import "LocationManager.h"

@class BalloonView;

@interface BookmarksVC : UITableViewController <LocationObserver>
{
  // @TODO store as a property to retain reference
  BalloonView * m_balloon;

  LocationManager * m_locationManager;
  size_t m_categoryIndex;
}

- (id) initWithBalloonView:(BalloonView *)view andCategory:(size_t)categoryIndex;

@end
