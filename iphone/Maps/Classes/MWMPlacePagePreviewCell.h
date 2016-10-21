#import "MWMTableViewCell.h"

@class MWMPlacePageData;
@class MWMCircularProgress;

@protocol MWMPlacePageCellUpdateProtocol;
@protocol MWMPlacePageLayoutDataSource;

@interface MWMPlacePagePreviewCell : MWMTableViewCell

- (void)setDistanceToObject:(NSString *)distance;
- (void)rotateDirectionArrowToAngle:(CGFloat)angle;

- (void)setDownloaderViewHidden:(BOOL)isHidden animated:(BOOL)isAnimated;
- (void)setDownloadingProgress:(CGFloat)progress;

- (void)configure:(MWMPlacePageData *)data
    updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
              dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource
               tapAction:(TMWMVoidBlock)tapAction;

- (MWMCircularProgress *)mapDownloadProgress;

@end
