#import "MWMTableViewCell.h"

@class MWMPlacePage;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)config:(MWMPlacePage *)placePage isReport:(BOOL)isReport;

@end
