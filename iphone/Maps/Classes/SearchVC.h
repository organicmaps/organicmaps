#import <UIKit/UIKit.h>

#import "LocationManager.h"

class Framework;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource,
    LocationObserver>
{
  Framework * m_framework;
  LocationManager * m_locationManager;
  UISearchBar * m_searchBar;
  UITableView * m_table;
  // Search is in progress indicator
  UIActivityIndicatorView * m_indicator;
  // View inside search bar which is replaced by indicator when it's active
  UIView * m_originalIndicatorView;
  // Used for direct access to replace indicator
  UITextField * m_searchTextField;
  // Zero when suggestions cells are not visible
  NSInteger m_suggestionsCount;
  NSArray * categoriesNames;
}
@property (nonatomic, retain) NSMutableArray * searchResults;
@end
