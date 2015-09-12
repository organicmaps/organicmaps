#import <UIKit/UIKit.h>

@class MWMPlacePageEntity, MWMDirectionView;

@interface MWMBasePlacePageView : UIView

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * typeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (weak, nonatomic) IBOutlet UIImageView * directionArrow;
@property (weak, nonatomic) IBOutlet UITableView * featureTable;
@property (weak, nonatomic) IBOutlet UIView * separatorView;
@property (weak, nonatomic) IBOutlet UIButton * directionButton;
@property (nonatomic) UIView * typeDescriptionView;

- (void)configureWithEntity:(MWMPlacePageEntity *)entity;
- (void)addBookmark;
- (void)removeBookmark;
- (void)reloadBookmarkCell;
- (void)updateAndLayoutMyPositionSpeedAndAltitude:(NSString *)text;

@end
