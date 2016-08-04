#import "MWMPlacePageEntity.h"
#import "MWMTableViewCell.h"

@interface MWMPlacePageInfoCell : MWMTableViewCell

- (void)configureWithType:(MWMPlacePageCellType)type info:(NSString *)info;

@property(weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readonly) IBOutlet id textContainer;
@property(nonatomic) MWMPlacePageEntity * currentEntity;

@end
