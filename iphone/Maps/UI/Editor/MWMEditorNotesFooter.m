#import "MWMEditorNotesFooter.h"
#import "SwiftBridge.h"

static void applyThemeRecursively(UIView * view)
{
  [view applyTheme];
  for (UIView * subview in view.subviews)
    applyThemeRecursively(subview);
}

@interface MWMEditorNotesFooter ()

@property(weak, nonatomic) UIViewController * controller;
@property(weak, nonatomic) IBOutlet UIButton * osmButton;

@end

@implementation MWMEditorNotesFooter

+ (instancetype)footerForController:(UIViewController *)controller
{
  MWMEditorNotesFooter * f = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  f.controller = controller;
  f.osmButton.titleLabel.numberOfLines = 0;
  f.osmButton.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
  applyThemeRecursively(f);
  [f setNeedsLayout];
  [f layoutIfNeeded];
  return f;
}

- (IBAction)osmTap
{
  [self.controller openUrl:L(@"osm_wiki_about_url")];
}

@end
