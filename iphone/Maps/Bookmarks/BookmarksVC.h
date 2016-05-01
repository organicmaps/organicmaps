#import "LocationManager.h"
#import "MWMTableViewController.h"

@interface BookmarksVC : MWMTableViewController <LocationObserver, UITextFieldDelegate>
{
  size_t m_categoryIndex;
}

- (instancetype)initWithCategory:(size_t)index;

@end
