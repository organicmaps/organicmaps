#import "MWMSearchResultController.h"
#import "MWMSearchResultCell.h"
#import "MWMWatchLocationTracker.h"
#import "MWMWatchEventInfo.h"
#import "MWMWatchNotification.h"
#import "MWMFrameworkUtils.h"
#import "Macros.h"

static NSString * const kSearchQueryNotificationId = @"SearchQueryNotificationId";
static NSString * const kSearchResultCellIdentifier = @"SearchResultCell";
extern NSString * const kNoLocationControllerIdentifier;

extern NSString * const kSearchResultTitleKey = @"searchResultTitle";
extern NSString * const kSearchResultCategoryKey = @"searchResultCategory";
extern NSString * const kSearchResultPointKey = @"searchResultPoint";

@interface MWMSearchResultController() <MWMWatchLocationTrackerDelegate>

@property (weak, nonatomic) IBOutlet WKInterfaceTable * table;

@property (nonatomic) NSMutableArray * searchResult;
@property (copy, nonatomic) NSString * query;

@end

@implementation MWMSearchResultController

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
  self.query = context;
  [self setTitle:L(self.query)];
  [MWMWatchLocationTracker sharedLocationTracker].delegate = self;
}

- (void)willActivate
{
  [super willActivate];
  self.searchResult = [NSMutableArray array];
  [self searchAroundCurrentLocation];
}

- (void)searchAroundCurrentLocation
{
  [MWMFrameworkUtils searchAroundCurrentLocation:self.query.precomposedStringWithCanonicalMapping callback:^(NSMutableArray * result)
  {
    [self.searchResult addObjectsFromArray:result];
    dispatch_async(dispatch_get_main_queue(),
    ^{
      [self.table setNumberOfRows:self.searchResult.count withRowType:kSearchResultCellIdentifier];
      [self reloadTable];
    });
  }];
}

- (void)reloadTable
{
  [self.searchResult enumerateObjectsUsingBlock:^(NSDictionary * d, NSUInteger idx, BOOL *stop)
  {
    MWMSearchResultCell * cell = [self.table rowControllerAtIndex:idx];
    [cell.titleLabel setText:d[kSearchResultTitleKey]];
    [cell.categoryLabel setText:d[kSearchResultCategoryKey]];
    m2::PointD featureCenter;
    [d[kSearchResultPointKey] getValue:&featureCenter];
    [self updateDistanceOnCell:cell withPoint:featureCenter];
  }];
}

- (void)table:(WKInterfaceTable *)table didSelectRowAtIndex:(NSInteger)rowIndex
{
  NSDictionary * d = self.searchResult[rowIndex];
  m2::PointD featureCenter;
  [d[kSearchResultPointKey] getValue:&featureCenter];
  [MWMWatchLocationTracker sharedLocationTracker].destinationPosition = featureCenter;
  [self popToRootController];
}

- (void)updateDistanceOnCell:(MWMSearchResultCell *)cell withPoint:(m2::PointD)point
{
  NSString * distance = [[MWMWatchLocationTracker sharedLocationTracker] distanceToPoint:point];
  [cell.distanceLabel setText:[NSString stringWithFormat:@"%@", distance]];
}

#pragma mark - MWMLocationTrackerDelegate

- (void)onChangeLocation:(CLLocation *)location
{
  [self.searchResult enumerateObjectsUsingBlock:^(NSDictionary * d, NSUInteger idx, BOOL *stop)
  {
    MWMSearchResultCell * cell = [self.table rowControllerAtIndex:idx];
    m2::PointD featureCenter;
    [d[kSearchResultPointKey] getValue:&featureCenter];
    [self updateDistanceOnCell:cell withPoint:featureCenter];
  }];
}

- (void)locationTrackingFailedWithError:(NSError *)error
{
  [self pushControllerWithName:kNoLocationControllerIdentifier context:nil];
}

@end
