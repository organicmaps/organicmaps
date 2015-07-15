
#import <UIKit/UIKit.h>

@interface SearchCell : UITableViewCell

@property (nonatomic) UIView * separatorView;
@property (nonatomic, readonly) UILabel * titleLabel;

- (void)setTitle:(NSString *)title selectedRanges:(NSArray *)selectedRanges;
- (void)configTitleLabel;

@end
