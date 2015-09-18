#import "MWMDownloadMapRequest.h"

@interface MWMDownloadMapRequestView : SolidTouchView

@property (nonatomic) enum MWMDownloadMapRequestState state;

- (nonnull instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("initWithFrame is not available")));
- (nonnull instancetype)init __attribute__((unavailable("init is not available")));

@end
