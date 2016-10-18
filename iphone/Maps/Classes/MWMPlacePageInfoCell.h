#import "MWMPlacePageEntity.h"
#import "MWMTableViewCell.h"

#import "MWMPlacePageData.h"

@interface MWMPlacePageInfoCell : MWMTableViewCell

- (void)configureWithType:(MWMPlacePageCellType)type
                     info:(NSString *)info NS_DEPRECATED_IOS(7_0, 8_0, "Use configWithRow:data: instead");

- (void)configWithRow:(place_page::MetainfoRows)row
                 data:(MWMPlacePageData *)data NS_AVAILABLE_IOS(8_0);

@property(weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readonly) IBOutlet id textContainer;
@property(nonatomic) MWMPlacePageEntity * currentEntity;

@end
