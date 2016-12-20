#import "MWMCircularProgress.h"

enum class MWMMigrationViewState
{
  Default,
  Processing,
  ErrorNoConnection,
  ErrorNoSpace
};

@interface MWMMigrationView : UIView

@property (nonatomic) MWMMigrationViewState state;
@property (copy, nonatomic) NSString * nodeLocalName;
@property (weak, nonatomic) id<MWMCircularProgressProtocol> delegate;

- (void)setProgress:(CGFloat)progress;

@end
