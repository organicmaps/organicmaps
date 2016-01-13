#import "MWMTableViewCell.h"

@class MWMPlacePageEntity;

@interface MWMPlacePageInfoCell : MWMTableViewCell

- (void)configureWithType:(MWMPlacePageMetadataType)type info:(NSString *)info;

@property (weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readonly) IBOutlet id textContainer;
@property (nonatomic) MWMPlacePageEntity * currentEntity;

@end
