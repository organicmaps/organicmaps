typedef NS_ENUM(NSUInteger, MWMDownloadMapRequestState)
{
  MWMDownloadMapRequestStateDownload,
  MWMDownloadMapRequestStateRequestLocation,
  MWMDownloadMapRequestStateRequestUnknownLocation
};

@protocol MWMDownloadMapRequestDelegate <NSObject>

- (void)stateUpdated:(enum MWMDownloadMapRequestState)state;
- (void)selectMapsAction;

@end

@interface MWMDownloadMapRequest : NSObject

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView
                                  delegate:(nonnull id <MWMDownloadMapRequestDelegate>)delegate;

- (void)showRequest;

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)setDownloadFailed;

@end
