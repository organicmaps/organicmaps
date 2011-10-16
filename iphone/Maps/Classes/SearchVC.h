#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

#include "../../map/framework.hpp"
#include "../../map/feature_vec_model.hpp"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

namespace search { class Result; }

typedef Framework<model::FeaturesFetcher> framework_t;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource, CLLocationManagerDelegate>
{
  framework_t * m_framework;
  vector<search::Result> m_results;
  CLLocationManager * m_locationManager;
  UISearchBar * m_searchBar;
  UITableView * m_table;
}

@property (nonatomic, retain) IBOutlet UISearchBar * m_searchBar;
@property (nonatomic, retain) IBOutlet UITableView * m_table;

- (id)initWithFramework:(framework_t *)framework;

@end
