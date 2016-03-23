#import "MWMViewController.h"

@protocol MWMOpeningHoursEditorProtocol <NSObject>

- (void)setOpeningHours:(NSString *)openingHours;

@end

@interface MWMOpeningHoursEditorViewController : MWMViewController

@property (copy, nonatomic) NSString * openingHours;
@property (weak, nonatomic) id<MWMOpeningHoursEditorProtocol> delegate;

@end
