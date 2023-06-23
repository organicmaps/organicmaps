#import "MapsAppDelegate.h"
#import "ClipboardMonitor.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import <CoreApi/DeepLinkParser.h>
#include <CoreApi/Framework.h>
#import <CoreApi/DeepLinkData.h>
#import "UIKitCategories.h"
#import <CoreApi/DeepLinkSearchData.h>

@interface ClipboardWatcher ()

@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, copy) NSString *previousClipboardText;
@property (nonatomic, assign) BOOL isErrorPromptShown;

@end

@implementation ClipboardWatcher

- (void)startWatching:(UIWindow *)window {
    NSLog(@"clip watching started");

    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    NSString *currentClipboardText = pasteboard.string;
    NSLog(@"clip clipboard checking for changes %@  and  %@",currentClipboardText, self.previousClipboardText);

    [pasteboard setString:@""];
    [self checkClipboardWithText:currentClipboardText window:window];
}

- (void)checkClipboardWithText:(NSString *)copiedText window:(UIWindow *)window {
    NSLog(@"clip API CALL PROCEEDING %@", copiedText);

    // Check if the copied text is a URL
    if ([self isURL:copiedText]) {

        dispatch_async(dispatch_get_main_queue(), ^{
            UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Redirecting from copied URL" message:@"Please wait..." preferredStyle:UIAlertControllerStyleAlert];
            [window.rootViewController presentViewController:alertController animated:YES completion:nil];
        });

        // Make an API request
        NSURL *apiURL = [NSURL URLWithString:[NSString stringWithFormat:@"https://url-un.kartikay-2101ce32.workers.dev/search?url=%@", copiedText]];
        NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithURL:apiURL completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [window.rootViewController dismissViewControllerAnimated:YES completion:nil];
            });

            if (error) {
                NSLog(@"API Request Error: %@", error);
                NSString *errorMessage = @"Kindly make sure that your internet connection in turned on.";
                [self displayErrorMessage:errorMessage inWindow:window];
            } else {
                NSError *jsonError;
                NSString *responseString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                NSLog(@"API Response: %@", responseString);
                NSDictionary *responseDict = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&jsonError];
                if (jsonError) {
                    NSLog(@"JSON Parsing Error: %@", jsonError);
                    NSString *errorMessage = @"Kindly make sure that your URL points to an address";
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

    for (NSTextCheckingResult *match in matches) {
        if (match.resultType == NSTextCheckingTypeLink) {
            return YES;
        }
    }

    return NO;
}

- (void)displayErrorMessage:(NSString *)errorMessage inWindow:(UIWindow *)window {
    if (!self.isErrorPromptShown) {
        self.isErrorPromptShown = YES;

        dispatch_async(dispatch_get_main_queue(), ^{
            UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Redirection Failed" message:errorMessage preferredStyle:UIAlertControllerStyleAlert];
            UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                self.isErrorPromptShown = NO;
            }];
            [alertController addAction:okAction];
            [window.rootViewController presentViewController:alertController animated:YES completion:nil];
        });
    }
}

@end
