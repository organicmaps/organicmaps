
enum class MWMMigrationViewState
{
  Default,
  Processing,
  ErrorNoConnection,
  ErrorNoSpace
};

@interface MWMMigrationView : UIView

@property (nonatomic) MWMMigrationViewState state;

@end
