
#import <UIKit/UIKit.h>
#import "SearchCell.h"

@interface SearchUniversalCell : SearchCell

@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) UILabel * subtitleLabel;
@property (nonatomic, readonly) UILabel * distanceLabel;

+ (CGFloat)cellHeightWithTitle:(NSString *)title subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width;

@end
