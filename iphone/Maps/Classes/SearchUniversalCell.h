
#import <UIKit/UIKit.h>
#import "SearchCell.h"

@interface SearchUniversalCell : SearchCell

@property (nonatomic, readonly) UILabel * titleLabel;
@property (nonatomic, readonly) UILabel * typeLabel;
@property (nonatomic, readonly) UILabel * subtitleLabel;
@property (nonatomic, readonly) UILabel * distanceLabel;
@property (nonatomic, readonly) UIImageView * iconImageView;
@property (nonatomic) BOOL largeIconStyle;

- (void)setTitle:(NSString *)title selectedRanges:(NSArray *)selectedRanges;

+ (CGFloat)cellHeightWithTitle:(NSString *)title type:(NSString *)type subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width;

@end
