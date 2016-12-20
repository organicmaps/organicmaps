#import "MWMTableViewCell.h"

#import "MWMPlacePageData.h"

@interface MWMPlacePageInfoCell : MWMTableViewCell

- (void)configWithRow:(place_page::MetainfoRows)row
                 data:(MWMPlacePageData *)data;

@property(weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readonly) IBOutlet id textContainer;

@end
