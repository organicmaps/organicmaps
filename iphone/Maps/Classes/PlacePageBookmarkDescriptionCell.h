
#import <UIKit/UIKit.h>

@interface PlacePageBookmarkDescriptionCell : UITableViewCell

+ (CGFloat)cellHeightWithWebViewHeight:(CGFloat)webViewHeight;
+ (CGFloat)cellHeightWithTextValue:(NSString *)text viewWidth:(CGFloat)viewWidth;


@property (nonatomic, weak) UIWebView * webView;
@property (nonatomic) UILabel * titleLabel;

@end
