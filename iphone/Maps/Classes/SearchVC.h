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
  // Zero when suggestions cells are not visible
  NSInteger m_suggestionsCount;
  NSArray * categoriesNames;
}
@property (nonatomic, retain) NSMutableArray * searchResults;
@end
