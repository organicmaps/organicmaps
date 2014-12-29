#import <UIKit/UIKit.h>
#import "TableViewController.h"
#import "LocationManager.h"

@interface BookmarksVC : TableViewController <LocationObserver, UITextFieldDelegate>
{
  LocationManager * m_locationManager;
  size_t m_categoryIndex;
}

- (id)initWithCategory:(size_t)index;

@end
