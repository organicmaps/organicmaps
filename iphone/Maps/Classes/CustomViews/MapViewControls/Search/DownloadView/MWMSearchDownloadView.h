typedef NS_ENUM(NSUInteger, MWMSearchDownloadViewState)
{
  MWMSearchDownloadViewStateProgress,
  MWMSearchDownloadViewStateRequest
};

@interface MWMSearchDownloadView : UIView

@property (nonatomic) enum MWMSearchDownloadViewState state;

@end
