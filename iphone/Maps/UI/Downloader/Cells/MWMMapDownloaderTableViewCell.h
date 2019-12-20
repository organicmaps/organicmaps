#import "MWMMapDownloaderMode.h"
#import "MWMTableViewCell.h"

@class MWMMapNodeAttributes;
@class MWMMapDownloaderTableViewCell;

NS_ASSUME_NONNULL_BEGIN

@protocol MWMMapDownloaderTableViewCellDelegate <NSObject>

- (void)mapDownloaderCellDidPressProgress:(MWMMapDownloaderTableViewCell *)cell;
- (void)mapDownloaderCellDidLongPress:(MWMMapDownloaderTableViewCell *)cell;

@end

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell

@property(weak, nonatomic) id<MWMMapDownloaderTableViewCellDelegate> delegate;
@property(nonatomic) MWMMapDownloaderMode mode;
@property(readonly, nonatomic) MWMMapNodeAttributes *nodeAttrs;

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(nullable NSString *)searchQuery;
- (void)setDownloadProgress:(CGFloat)progress;

@end

NS_ASSUME_NONNULL_END
