#import "MWMButtonCell.h"

@interface MWMButtonCell ()

@property(nonatomic) IBOutlet UIButton * button;
@property(weak, nonatomic) id<MWMButtonCellDelegate> delegate;

@end

@implementation MWMButtonCell

- (void)configureWithDelegate:(id<MWMButtonCellDelegate>)delegate title:(NSString *)title enabled:(BOOL)enabled
{
  self.button.titleLabel.numberOfLines = 0;
  self.button.titleLabel.textAlignment = NSTextAlignmentCenter;
  self.button.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  [self.button setTitle:title forState:UIControlStateNormal];
  self.button.enabled = enabled;
  self.delegate = delegate;
}

- (IBAction)buttonTap
{
  [self.delegate cellDidPressButton:self];
}

@end
