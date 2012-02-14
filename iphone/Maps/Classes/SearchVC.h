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
  UIButton * m_radarButton;
  UISearchBar * m_searchBar;
  UITableView * m_table;
  string m_currentCountryFlagCode;
}

- (id)initWithFramework:(Framework *)framework andLocationManager:(LocationManager *)lm;

@end
