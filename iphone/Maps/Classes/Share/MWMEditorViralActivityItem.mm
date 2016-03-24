#import "MWMEditorViralActivityItem.h"

#include "platform/preferred_languages.hpp"

@implementation MWMEditorViralActivityItem

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return @"";
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  if ([activityType isEqualToString:UIActivityTypePostToFacebook] ||
      [activityType isEqualToString:@"com.facebook.Facebook.ShareExtension"] ||
      [activityType.lowercaseString rangeOfString:@"facebook"].length)
  {
    NSString * url = [NSString stringWithFormat:@"http://maps.me/fb-editor-v1?lang=%@",
                      @(languages::GetCurrentNorm().c_str())];
    return [NSURL URLWithString:url];
  }

  if ([activityType isEqualToString:UIActivityTypeMail])
    return L(@"whatsnew_editor_message_1");

  return [NSString stringWithFormat:@"%@\n%@", L(@"editor_sharing_title"), L(@"whatsnew_editor_message_1")];
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return L(@"editor_sharing_title");
}

@end
