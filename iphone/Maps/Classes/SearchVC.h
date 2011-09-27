#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

#include "../../map/framework.hpp"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

namespace search { class Result; }

typedef function<void (string const &, SearchCallbackT)> SearchF;
typedef function<void (m2::RectD)> ShowRectF;
typedef function<m2::PointD (void)> GetViewportCenterF;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource, CLLocationManagerDelegate>
{
  vector<search::Result> m_results;
  CLLocationManager * m_locationManager;
  bool m_isRadarEnabled;
}

- (id)initWithSearchFunc:(SearchF)s andShowRectFunc:(ShowRectF)r
    andGetViewportCenterFunc:(GetViewportCenterF)c;

@end
