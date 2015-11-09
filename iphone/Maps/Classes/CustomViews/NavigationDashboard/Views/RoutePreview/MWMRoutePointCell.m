#import "MWMRoutePointCell.h"

@interface MWMRoutePointCell () <UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIView * moveView;

@end

@implementation MWMRoutePointCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  UIPanGestureRecognizer * pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(didPan:)];
  [self.moveView addGestureRecognizer:pan];
}

- (void)didPan:(UIPanGestureRecognizer *)sender
{
  [self.delegate didPan:sender cell:self];
}

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
  [self.delegate startEditingCell:self];
  return NO;
}

@end
