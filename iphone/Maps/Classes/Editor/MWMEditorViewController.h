#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "TableViewController.h"

@interface MWMEditorViewController : TableViewController <MWMOpeningHoursEditorProtocol>

@property (nonatomic) MWMPlacePageEntity * entity;

- (void)setOpeningHours:(NSString *)openingHours;

@end
