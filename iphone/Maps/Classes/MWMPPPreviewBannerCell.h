@class MWMPlacePageData;

@interface MWMPPPreviewBannerCell : UITableViewCell

- (void)configWithData:(MWMPlacePageData *)data;

- (void)configImageInOpenState;
- (void)configImageInPreviewState;

@end
