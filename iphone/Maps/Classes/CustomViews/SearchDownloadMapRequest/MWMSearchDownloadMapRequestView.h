
typedef NS_ENUM(NSUInteger, MWMSearchDownloadMapRequestViewState)
{
  MWMSearchDownloadMapRequestViewStateProgress,
  MWMSearchDownloadMapRequestViewStateRequest
};

@interface MWMSearchDownloadMapRequestView : SolidTouchView

- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (instancetype)init __attribute__((unavailable("init is not available")));

- (void)show:(enum MWMSearchDownloadMapRequestViewState)state;

@end
