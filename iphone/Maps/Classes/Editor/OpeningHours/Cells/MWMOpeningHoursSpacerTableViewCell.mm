#import "MWMOpeningHoursSpacerTableViewCell.h"

@implementation MWMOpeningHoursSpacerTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width
{
  return 40.0;
}

- (void)awakeFromNib
{
  self.backgroundColor = [UIColor clearColor];
}

@end
