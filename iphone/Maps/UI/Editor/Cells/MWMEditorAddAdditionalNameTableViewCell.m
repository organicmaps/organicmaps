#import "MWMEditorAddAdditionalNameTableViewCell.h"

@interface MWMEditorAddAdditionalNameTableViewCell ()

@property(weak, nonatomic) IBOutlet UIButton * button;
@property(weak, nonatomic) id<MWMEditorAdditionalName> delegate;

@end

@implementation MWMEditorAddAdditionalNameTableViewCell

- (void)configWithDelegate:(id<MWMEditorAdditionalName>)delegate
{
  self.button.titleLabel.numberOfLines = 0;
  self.button.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  self.delegate = delegate;
}
- (IBAction)addLanguageTap
{
  [self.delegate editAdditionalNameLanguage:NSNotFound];
}
@end
