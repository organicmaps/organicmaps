#import "MWMOpeningHoursSection.h"
#import "MWMTableViewCell.h"

@interface MWMOpeningHoursTableViewCell : MWMTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width;

@property (nonatomic, readonly) NSUInteger row;
@property (weak, nonatomic) MWMOpeningHoursSection * section;
@property (nonatomic) NSIndexPath * indexPathAtInit;
@property (nonatomic, readonly) BOOL isVisible;

- (void)hide;

- (void)refresh;

@end
