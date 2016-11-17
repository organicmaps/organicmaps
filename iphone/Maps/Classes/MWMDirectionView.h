#import <UIKit/UIKit.h>

@class MWMPlacePageViewManager;

@interface MWMDirectionView : UIView

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * typeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (weak, nonatomic) IBOutlet UIImageView * directionArrow;
@property (weak, nonatomic) IBOutlet UIView * contentView;

- (void)setDirectionArrowTransform:(CGAffineTransform)transform;

- (void)show;

- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("initWithCoder is not available")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));

@end
