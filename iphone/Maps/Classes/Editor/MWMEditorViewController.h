#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMTableViewController.h"

@interface MWMEditorViewController : MWMTableViewController <MWMOpeningHoursEditorProtocol>

@property (nonatomic) MWMPlacePageEntity * entity;

- (void)setOpeningHours:(NSString *)openingHours;

@end
