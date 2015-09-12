#import <UIKit/UIKit.h>
#import "MWMPlacePageEntity.h"

@interface MWMPlacePageInfoCell : UITableViewCell

- (void)configureWithType:(MWMPlacePageMetadataType)type info:(NSString *)info;

@property (weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readonly) IBOutlet id textContainer;
@property (nonatomic) MWMPlacePageEntity * currentEntity;

@end
