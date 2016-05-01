#import "MWMEditorNotesFooter.h"

@implementation MWMEditorNotesFooter

+ (instancetype)footer
{
  MWMEditorNotesFooter * f = [[[NSBundle mainBundle] loadNibNamed:[MWMEditorNotesFooter className] owner:nil options:nil]
                                firstObject];
  [f setNeedsLayout];
  [f layoutIfNeeded];
  NSAssert(f.subviews.firstObject, @"Subviews can't be empty!");
  f.height = f.subviews.firstObject.height;
  return f;
}

- (IBAction)osmTap
{
  NSURL * url = [NSURL URLWithString:@"https://wiki.openstreetmap.org/wiki/Main_Page"];
  UIApplication * app = [UIApplication sharedApplication];
  if ([app canOpenURL:url])
    [app openURL:url];
}

@end
