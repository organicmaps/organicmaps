#import <UIKit/UIKit.h>

@interface MWMBookmarkColorCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UIButton * colorButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UIImageView * approveImageView;

- (void)configureWithColorString:(NSString *)colorString;

@end
