#import "MWMMapDownloaderMode.h"
#import "MWMTableViewCell.h"

@class MWMCircularProgress;
@class MWMMapNodeAttributes;
@class MWMMapDownloaderTableViewCell;

NS_ASSUME_NONNULL_BEGIN

@protocol MWMMapDownloaderTableViewCellDelegate <NSObject>

- (void)mapDownloaderCellDidPressProgress:(MWMMapDownloaderTableViewCell *)cell;
- (void)mapDownloaderCellDidLongPress:(MWMMapDownloaderTableViewCell *)cell;

@end

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell

@property(nonatomic) MWMCircularProgress * progress;
@property(weak, nonatomic) id<MWMMapDownloaderTableViewCellDelegate> delegate;
@property(nonatomic) MWMMapDownloaderMode mode;
@property(readonly, nonatomic) MWMMapNodeAttributes * nodeAttrs;
/// True for the synthetic terrain (.twm) sub-row of the region.
@property(readonly, nonatomic) BOOL isTerrainCell;

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(nullable NSString *)searchQuery;
/// Configures the cell as the terrain sub-row of nodeAttrs' region.
- (void)configTerrain:(MWMMapNodeAttributes *)nodeAttrs;
- (void)configProgress:(MWMMapNodeAttributes *)nodeAttrs;
- (void)setDownloadProgress:(CGFloat)progress;
- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs;
@end

NS_ASSUME_NONNULL_END
