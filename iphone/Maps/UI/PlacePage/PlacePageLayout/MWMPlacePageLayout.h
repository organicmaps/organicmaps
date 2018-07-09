namespace storage
{
enum class NodeStatus;
}  // namespace storage

@protocol MWMPlacePageLayoutDelegate<NSObject>

- (void)onPlacePageTopBoundChanged:(CGFloat)bound;
- (void)destroyLayout;
- (void)closePlacePage;

- (BOOL)isExpandedOnShow;
- (void)onExpanded;

@end

@protocol MWMPlacePageLayoutDataSource<NSObject>

- (NSString *)distanceToObject;
- (void)downloadSelectedArea;

@end

@class MWMPlacePageData, MWMPPView;
@protocol MWMPlacePageButtonsProtocol, MWMActionBarProtocol;

/// Helps with place page view layout and representation
@interface MWMPlacePageLayout : NSObject

- (instancetype)initWithOwnerView:(UIView *)view
                         delegate:(id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol,
                                      MWMActionBarProtocol>)delegate
                       dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource;

- (void)showWithData:(MWMPlacePageData *)data;
- (void)close;

- (void)mwm_refreshUI;

- (UIView *)shareAnchor;

- (void)reloadBookmarkSection:(BOOL)isBookmark;

- (void)rotateDirectionArrowToAngle:(CGFloat)angle;
- (void)setDistanceToObject:(NSString *)distance;
- (void)setSpeedAndAltitude:(NSString *)speedAndAltitude;

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress;

- (void)checkCellsVisible;

- (void)updateAvailableArea:(CGRect)frame;

@end

