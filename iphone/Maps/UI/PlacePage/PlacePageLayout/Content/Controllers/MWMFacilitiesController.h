#import "MWMTableViewController.h"

#include <vector>

namespace booking
{
struct HotelFacility;
}

@interface MWMFacilitiesController : MWMTableViewController

- (void)setHotelName:(NSString *)name;
- (void)setFacilities:(std::vector<booking::HotelFacility> const &)facilities;

@end
