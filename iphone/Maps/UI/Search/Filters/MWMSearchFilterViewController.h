#import "MWMTableViewController.h"

#include "search/hotels_filter.hpp"

@interface MWMSearchFilterViewController : MWMTableViewController

+ (MWMSearchFilterViewController *)controller;

- (shared_ptr<search::hotels_filter::Rule>)rules;

- (void)reset;

@end
