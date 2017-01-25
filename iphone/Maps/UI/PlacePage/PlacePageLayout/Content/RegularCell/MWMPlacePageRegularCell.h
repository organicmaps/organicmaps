#import "MWMTableViewCell.h"

#import "MWMPlacePageData.h"

@interface MWMPlacePageRegularCell : MWMTableViewCell

- (void)configWithRow:(place_page::MetainfoRows)row data:(MWMPlacePageData *)data;

@property(weak, nonatomic, readonly) IBOutlet UIImageView * icon;
@property(weak, nonatomic, readonly) IBOutlet id textContainer;

@end

@interface MWMPlacePageInfoCell : MWMPlacePageRegularCell
@end

@interface MWMPlacePageLinkCell : MWMPlacePageRegularCell
@end
