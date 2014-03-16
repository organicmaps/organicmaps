
#import <UIKit/UIKit.h>
#import "SearchActivityProtocol.h"

@class SegmentedControl;
@protocol SegmentedControlDelegate <NSObject>

- (void)segmentedControl:(SegmentedControl *)segmentControl didSelectSegment:(NSInteger)segmentIndex;

@end

@interface SegmentedControl : UIView <SearchActivityProtocol>

- (void)setActive:(BOOL)active animated:(BOOL)animated;
@property (nonatomic) NSInteger selectedSegmentIndex;

@property (nonatomic) id <SegmentedControlDelegate> delegate;

- (NSInteger)segmentsCount;

@end
