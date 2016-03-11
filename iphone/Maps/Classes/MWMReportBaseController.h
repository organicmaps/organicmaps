#import "MWMTableViewController.h"

#include "geometry/point2d.hpp"

@interface MWMReportBaseController : MWMTableViewController

- (void)configNavBar NS_REQUIRES_SUPER;
- (void)sendNote:(string const &)note NS_REQUIRES_SUPER;

- (void)setPoint:(m2::PointD const &)point NS_REQUIRES_SUPER;
- (m2::PointD const &)point;

@end
