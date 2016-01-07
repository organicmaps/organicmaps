#import "ViewController.h"

@protocol MWMOpeningHoursEditorProtocol <NSObject>

- (void)setOpeningHours:(NSString *)openingHours;

@end

@interface MWMOpeningHoursEditorViewController : ViewController

@property (copy, nonatomic) NSString * openingHours;
@property (weak, nonatomic) id<MWMOpeningHoursEditorProtocol> delegate;

@end
