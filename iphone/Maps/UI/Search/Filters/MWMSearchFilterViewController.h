#import "MWMViewController.h"

#include "map/booking_filter_availability_params.hpp"

#include "std/shared_ptr.hpp"

namespace search
{
namespace hotels_filter
{
struct Rule;
}  // namespace hotels_filter
}  // namespace search

@interface MWMSearchFilterViewController : MWMViewController

+ (MWMSearchFilterViewController *)controller;

- (shared_ptr<search::hotels_filter::Rule>)rules;
- (booking::filter::availability::Params)availabilityParams;

@end
