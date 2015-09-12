#import <UIKit/UIKit.h>

@class MWMPlacePageEntity, MWMPlacePage, MWMTextView;

@interface MWMPlacePageBookmarkCell : UITableViewCell

@property (weak, nonatomic, readonly) IBOutlet UITextField * title;
@property (weak, nonatomic, readonly) IBOutlet UIButton * categoryButton;
@property (weak, nonatomic, readonly) IBOutlet UIButton * markButton;
@property (weak, nonatomic, readonly) IBOutlet UILabel * descriptionLabel;
@property (weak, nonatomic) UITableView * ownerTableView;
@property (weak, nonatomic) MWMPlacePage * placePage;

- (void)configure;

@end
