#import "MWMEditorNotesFooter.h"

@interface MWMEditorNotesFooter ()

@property(weak, nonatomic) UIViewController * controller;

@end

@implementation MWMEditorNotesFooter

+ (instancetype)footerForController:(UIViewController *)controller
{
  MWMEditorNotesFooter * f = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  f.controller = controller;
  [f setNeedsLayout];
  [f layoutIfNeeded];
  NSAssert(f.subviews.firstObject, @"Subviews can't be empty!");
  f.height = f.subviews.firstObject.height;
  return f;
}

- (IBAction)osmTap
{
  [self.controller openUrl:L(@"osm_wiki_about_url")];
}

@end
