@protocol MWMSearchDownloadMapRequest <NSObject>

- (void)selectMapsAction;

@end

@interface MWMSearchDownloadMapRequest : NSObject

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMSearchDownloadMapRequest>)delegate;

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)setDownloadFailed;

@end
