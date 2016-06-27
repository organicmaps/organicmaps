#import "MWMButton.h"
#import "MWMEditorAdditionalNamesHeader.h"
#import "UILabel+RuntimeAttributes.h"

@interface MWMEditorAdditionalNamesHeader ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (copy, nonatomic) TMWMVoidBlock toggleBlock;
@property (weak, nonatomic) IBOutlet MWMButton * toggleButton;

@end

@implementation MWMEditorAdditionalNamesHeader

+ (instancetype)header:(TMWMVoidBlock)toggleBlock
{
  MWMEditorAdditionalNamesHeader * h = [[[NSBundle mainBundle] loadNibNamed:[MWMEditorAdditionalNamesHeader className] owner:nil options:nil]
          firstObject];
  h.label.localizedText = L(@"editor_international_names_subtitle").uppercaseString;
  h.toggleBlock = toggleBlock;
  return h;
}

- (IBAction)toggleAction
{
  self.toggleBlock();
}

- (void)setShowAdditionalNames:(BOOL)showAdditionalNames
{
  [self.toggleButton setTitle:showAdditionalNames ? L(@"hide") : L(@"show") forState:UIControlStateNormal];
}

- (void)setAdditionalNamesVisible:(BOOL)visible
{
  self.toggleButton.hidden = !visible;
}

@end
