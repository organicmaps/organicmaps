#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "ViewController.h"

@interface MWMEditorViewController : ViewController <MWMOpeningHoursEditorProtocol>

@property (nonatomic) MWMPlacePageEntity * entity;

- (void)setOpeningHours:(NSString *)openingHours;

@end
