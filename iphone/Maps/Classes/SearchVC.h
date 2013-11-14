#import <UIKit/UIKit.h>
#import "LocationManager.h"
#import "ScopeView.h"

class Framework;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource,
    LocationObserver>
{
  Framework * m_framework;
  LocationManager * m_locationManager;
  UITableView * m_table;
  // Zero when suggestions cells are not visible
  NSInteger m_suggestionsCount;
  NSArray * categoriesNames;
}

@property NSMutableArray * searchResults;

@property (nonatomic) UISearchBar *searchBar;
@property (nonatomic) ScopeView *scopeView;

@end
