#include "storage/storage_defines.hpp"

@protocol MWMPlacePageLayoutDelegate<NSObject>

- (void)onTopBoundChanged:(CGFloat)bound;
- (void)shouldDestroyLayout;

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

- (void)layoutWithSize:(CGSize const &)size;
- (void)showWithData:(MWMPlacePageData *)data;
- (void)close;

- (void)mwm_refreshUI;

- (UIView *)shareAnchor;

- (void)reloadBookmarkSection:(BOOL)isBookmark;

- (void)rotateDirectionArrowToAngle:(CGFloat)angle;
- (void)setDistanceToObject:(NSString *)distance;

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress;

@end
