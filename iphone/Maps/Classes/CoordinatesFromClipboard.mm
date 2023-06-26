#import "MapsAppDelegate.h"
#import "CoordinatesFromClipboard.h"
#import "UIKitCategories.h"
#include <CoreApi/Framework.h>

@interface ClipboardCheck ()

@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, copy) NSString *previousClipboardText;
@property (nonatomic, assign) BOOL isErrorPromptShown;

@end

@implementation ClipboardCheck

NSString * privacyProxy = @"https://url-un.kartikay-2101ce32.workers.dev/coordinates?url=%@";
NSString *localizedStringOk = NSLocalizedString(@"ok", @"");
- (void)Check:(UIWindow *)window {
  UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
  NSString *currentClipboardText = pasteboard.string;
  NSLog(@"Local String%@", localizedStringOk);
  [self checkClipboardWithText:currentClipboardText window:window];
}
- (void)checkClipboardWithText:(NSString *)copiedText window:(UIWindow *)window {
  NSLog(@"PROXY REQUEST PROCEEDING %@", copiedText);
  
  // Check if the copied text is a URL
  if ([self isURL:copiedText]) {
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    [pasteboard setString:@""];
    NSLog(privacyProxy,copiedText);
    dispatch_async(dispatch_get_main_queue(), ^{
      NSString *message = [NSString stringWithFormat:@"Extracting coordinates from %@ using the anonymous proxy...", copiedText];
      UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Please Wait" message:message preferredStyle:UIAlertControllerStyleAlert];
      [window.rootViewController presentViewController:alertController animated:YES completion:nil];
    });
    
    // Make an API request
    NSURL *apiURL = [NSURL URLWithString:[NSString stringWithFormat:privacyProxy, copiedText]];
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithURL:apiURL completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [window.rootViewController dismissViewControllerAnimated:YES completion:nil];
      });
      
      if (error) {
        NSLog(@"Proxy request error: %@", error);
        NSString *errorMessage = @"Failed to extract coordinates from %url using an anonymous proxy. Please check your Internet connection";
        errorMessage = [errorMessage stringByReplacingOccurrencesOfString:@"%url" withString:copiedText];
        [self displayErrorMessage:errorMessage inWindow:window];
      } else {
        NSError *jsonError;
        NSString *responseString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSLog(@"Proxy response:%@", responseString);
        NSDictionary *responseDict = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&jsonError];
        if (jsonError) {
          NSLog(@"JSON Parsing Error: %@", jsonError);
          NSString *errorMessage = @"Can not extract coordinates from %url";
          errorMessage = [errorMessage stringByReplacingOccurrencesOfString:@"%url" withString:copiedText];
          [self displayErrorMessage:errorMessage inWindow:window];
        } else {
          NSDictionary *urlDict = responseDict[@"url"];
          NSString *geoURL = urlDict[@"geo"];
          if (geoURL) {
            NSLog(@"Geo URL: %@", geoURL);
            dispatch_async(dispatch_get_main_queue(), ^{
              GetFramework().ShowMapForURL(geoURL.UTF8String);
            });
          }
        }
      }
    }];
    [task resume];
  }
}

- (BOOL)isURL:(NSString *)text {
  NSDataDetector *detector = [NSDataDetector dataDetectorWithTypes:NSTextCheckingTypeLink error:nil];
  NSArray *matches = [detector matchesInString:text options:0 range:NSMakeRange(0, text.length)];
  
  NSArray *googleHosts = @[@"google", @"google.com", @"goo.gl", @"maps.app.goo.gl"];
  
  for (NSTextCheckingResult *match in matches) {
    NSURL *url = match.URL;
    NSString *hostname = url.host;
    
    for (NSString *googleHost in googleHosts) {
      if ([hostname containsString:googleHost]) return YES;
    }
  }
  return NO;
}

- (void)displayErrorMessage:(NSString *)errorMessage inWindow:(UIWindow *)window {
  if (!self.isErrorPromptShown) {
    self.isErrorPromptShown = YES;
    
    dispatch_async(dispatch_get_main_queue(), ^{
      NSString *title = @"Redirection to the coordinates failed";
      UIAlertController *alertController = [UIAlertController alertControllerWithTitle:title message:errorMessage preferredStyle:UIAlertControllerStyleAlert];
      UIAlertAction *okAction = [UIAlertAction actionWithTitle:localizedStringOk style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        self.isErrorPromptShown = NO;
      }];
      [alertController addAction:okAction];
      [window.rootViewController presentViewController:alertController animated:YES completion:nil];
    });
  }
}

@end
