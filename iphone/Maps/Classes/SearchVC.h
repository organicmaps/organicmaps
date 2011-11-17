#import <UIKit/UIKit.h>

#import "LocationManager.h"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

@class LocationManager;

class Framework;
namespace search { class Result; }

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource, LocationObserver>
{
  Framework * m_framework;
  LocationManager * m_locationManager;
  vector<search::Result> m_results;
  UISearchBar * m_searchBar;
  UITableView * m_table;
//  UILabel * m_warningView;
  /// Warning view shows only if this text is not nil
//  NSString * m_warningViewText;
}

- (id)initWithFramework:(Framework *)framework andLocationManager:(LocationManager *)lm;

@end
