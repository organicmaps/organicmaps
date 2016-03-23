#import "LocationManager.h"
#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <LocationObserver, UITextFieldDelegate>
{
  LocationManager * m_locationManager;
  size_t m_categoryIndex;
}

- (id)initWithCategory:(size_t)index;

@end
