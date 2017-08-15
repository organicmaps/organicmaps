#import "MWMTableViewController.h"

#include "std/shared_ptr.hpp"

namespace search
{
namespace hotels_filter
{
struct Rule;
}  // namespace hotels_filter
}  // namespace search

@interface MWMSearchFilterViewController : MWMTableViewController

+ (MWMSearchFilterViewController *)controller;

- (shared_ptr<search::hotels_filter::Rule>)rules;

- (void)reset;

@end
