#import "MWMTableViewController.h"

@class HotelFacility;

@interface MWMFacilitiesController : MWMTableViewController

@property(nonatomic, copy) NSString *name;
@property(nonatomic, copy) NSArray<HotelFacility *> *facilities;

@end
