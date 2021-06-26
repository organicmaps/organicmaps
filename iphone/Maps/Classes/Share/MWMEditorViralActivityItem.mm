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
    NSString * url = [NSString stringWithFormat:@"https://omaps.app/fb-editor-v1?lang=%@",
                      @(languages::GetCurrentNorm().c_str())];
    return [NSURL URLWithString:url];
  }

  NSString * omapsURL = @"https://omaps.app/get";
  if ([activityType isEqualToString:UIActivityTypePostToTwitter] || [activityType isEqualToString:UIActivityTypeMail])
    return [NSString stringWithFormat:@"%@ %@", L(@"whatsnew_editor_message_1"), omapsURL];

  return [NSString stringWithFormat:@"%@.\n%@\n%@", L(@"editor_sharing_title"), L(@"whatsnew_editor_message_1"), omapsURL];
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return L(@"editor_sharing_title");
}

@end
