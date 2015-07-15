
#import <UIKit/UIKit.h>
#import "SearchCell.h"

@interface SearchResultCell : SearchCell

@property (nonatomic, readonly) UILabel * typeLabel;
@property (nonatomic, readonly) UILabel * subtitleLabel;
@property (nonatomic, readonly) UILabel * distanceLabel;
@property (nonatomic, readonly) UIImageView * iconImageView;

+ (CGFloat)cellHeightWithTitle:(NSString *)title type:(NSString *)type subtitle:(NSString *)subtitle distance:(NSString *)distance viewWidth:(CGFloat)width;

@end
