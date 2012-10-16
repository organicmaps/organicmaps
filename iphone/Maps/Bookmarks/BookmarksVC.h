#import <UIKit/UIKit.h>
#import "LocationManager.h"

@class BalloonView;

@interface BookmarksVC : UITableViewController <LocationObserver, UITextFieldDelegate>
{
  LocationManager * m_locationManager;
  size_t m_categoryIndex;
}

- (id) initWithCategory:(size_t)index;

@end
