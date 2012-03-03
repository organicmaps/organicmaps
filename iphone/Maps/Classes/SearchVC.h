#import <UIKit/UIKit.h>

#import "LocationManager.h"
#import "SearchSuggestionsCell.h"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

@class LocationManager;

class Framework;
namespace search { class Result; }

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource,
    LocationObserver, SearchSuggestionDelegate>
{
  Framework * m_framework;
  LocationManager * m_locationManager;
  UISearchBar * m_searchBar;
  UITableView * m_table;
  // Zero when suggestions cells are not visible
  NSInteger m_suggestionsCount;
}

- (id)initWithFramework:(Framework *)framework andLocationManager:(LocationManager *)lm;

@end
