#import "MWMEditorSelectTableViewCell.h"
#import "UIColor+MapsMeColor.h"

@interface MWMEditorSelectTableViewCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * labelLeadingOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * labelTrailingOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * bottomSeparatorLeadingOffset;

@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMEditorSelectTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
               placeholder:(NSString *)placeholder
                  lastCell:(BOOL)lastCell
{
  self.delegate = delegate;
  self.icon.hidden = NO;
  self.icon.image = icon;
  if (text && text.length != 0)
  {
    self.label.text = text;
    self.label.textColor = [UIColor blackPrimaryText];
  }
  else
  {
    self.label.text = placeholder;
    self.label.textColor = [UIColor blackSecondaryText];
  }
  self.label.preferredMaxLayoutWidth = self.width - self.labelLeadingOffset.constant - self.labelTrailingOffset.constant;
  self.bottomSeparatorLeadingOffset.priority = lastCell ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
}

- (IBAction)selectAction
{
  [self.delegate actionForCell:self];
}

@end
