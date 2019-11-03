#import "MWMEditorSelectTableViewCell.h"
#import <CoreApi/MWMCommon.h>
#import "UIImageView+Coloring.h"

@interface MWMEditorSelectTableViewCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * labelLeadingOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * labelTrailingOffset;
@property(weak, nonatomic) IBOutlet UIImageView * grayArrow;

@property(weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorSelectTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
{
  self.delegate = delegate;
  self.icon.hidden = NO;
  self.icon.image = icon;
  self.icon.mwm_coloring = MWMImageColoringBlack;
  if (text && text.length != 0)
  {
    self.label.text = text;
    self.label.textColor = [UIColor blackPrimaryText];
  }
  else
  {
    self.label.text = placeholder;
    self.label.textColor = [UIColor blackHintText];
  }
  self.label.preferredMaxLayoutWidth =
      self.width - self.labelLeadingOffset.constant - self.labelTrailingOffset.constant;

  if (isInterfaceRightToLeft())
    self.grayArrow.transform = CGAffineTransformMakeScale(-1, 1);
}

- (IBAction)selectAction { [self.delegate cellSelect:self]; }
@end
