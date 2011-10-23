#import <UIKit/UIKit.h>

#import "LocationManager.h"

#include "../../map/framework.hpp"
#include "../../map/feature_vec_model.hpp"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

@class LocationManager;

namespace search { class Result; }

typedef Framework<model::FeaturesFetcher> framework_t;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource, LocationObserver>
{
  framework_t * m_framework;
  LocationManager * m_locationManager;
  vector<search::Result> m_results;
//  UILabel * m_warningView;
  /// Warning view shows only if this text is not nil
//  NSString * m_warningViewText;
}

@property (nonatomic, retain) IBOutlet UISearchBar * m_searchBar;
@property (nonatomic, retain) IBOutlet UITableView * m_table;

- (id)initWithFramework:(framework_t *)framework andLocationManager:(LocationManager *)lm;

@end
