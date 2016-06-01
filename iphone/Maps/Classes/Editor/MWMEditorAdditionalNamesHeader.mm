#import "MWMEditorAdditionalNamesHeader.h"
#import "UILabel+RuntimeAttributes.h"

@interface MWMEditorAdditionalNamesHeader ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (copy, nonatomic) TMWMVoidBlock toggleBlock;

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

- (IBAction)toggleAction:(UIButton *)sender
{
  NSString * newTitle = (sender.currentTitle == L(@"hide") ? L(@"show") : L(@"hide"));
  [sender setTitle:newTitle forState:UIControlStateNormal];
  self.toggleBlock();
}

@end
