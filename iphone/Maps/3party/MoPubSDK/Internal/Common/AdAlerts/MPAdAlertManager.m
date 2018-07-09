//
//  MPAdAlertManager.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAdAlertManager.h"
#import "MPAdConfiguration.h"
#import "MPAdAlertGestureRecognizer.h"
#import "MPLogging.h"
#import "MPIdentityProvider.h"
#import "MPCoreInstanceProvider.h"
#import "MPLastResortDelegate.h"

#import <QuartzCore/QuartzCore.h>
#import <CoreLocation/CoreLocation.h>
#import <MessageUI/MessageUI.h>

#define kTimestampParamKey @"timestamp"

@interface MPAdAlertManager () <UIGestureRecognizerDelegate, MFMailComposeViewControllerDelegate>

@property (nonatomic, assign) BOOL processedAlert;
@property (nonatomic, strong) MPAdAlertGestureRecognizer *adAlertGestureRecognizer;
@property (nonatomic, strong) MFMailComposeViewController *currentOpenMailVC;

@end

@implementation MPAdAlertManager

@synthesize delegate = _delegate;
@synthesize adConfiguration = _adConfiguration;
@synthesize processedAlert = _processedAlert;
@synthesize adAlertGestureRecognizer = _adAlertGestureRecognizer;
@synthesize adUnitId = _adUnitId;
@synthesize targetAdView = _targetAdView;
@synthesize location = _location;
@synthesize currentOpenMailVC = _currentOpenMailVC;

- (id)init
{
    self = [super init];
    if (self != nil) {
        self.adAlertGestureRecognizer = [[MPCoreInstanceProvider sharedProvider] buildMPAdAlertGestureRecognizerWithTarget:self
                                                                                                                action:@selector(handleAdAlertGesture)];
        self.adAlertGestureRecognizer.delegate = self;
        self.processedAlert = NO;
    }

    return self;
}

- (void)dealloc
{
    [_targetAdView removeGestureRecognizer:_adAlertGestureRecognizer];
    [_adAlertGestureRecognizer removeTarget:self action:nil];
    _adAlertGestureRecognizer.delegate = nil;
    _currentOpenMailVC.mailComposeDelegate = [MPLastResortDelegate sharedDelegate];
}

- (void)processAdAlert
{
    static NSDateFormatter *dateFormatter = nil;

    MPLogInfo(@"MPAdAlertManager processing ad alert");

    // don't even try if this device can't send emails
    if (![MFMailComposeViewController canSendMail]) {
        if ([self.delegate respondsToSelector:@selector(adAlertManagerDidProcessAlert:)]) {
            [self.delegate adAlertManagerDidProcessAlert:self];
        }

        return;
    }

    // since iOS 4, drawing an image to a graphics context is thread-safe
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        // take screenshot of the ad
        UIGraphicsBeginImageContextWithOptions(self.targetAdView.bounds.size, YES, 0.0);
        [self.targetAdView.layer renderInContext:UIGraphicsGetCurrentContext()];

        UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();

        dispatch_async(dispatch_get_main_queue(), ^{
            // package additional ad data
            NSMutableDictionary *params = [NSMutableDictionary dictionary];

            [params setValue:@"iOS" forKey:@"platform"];
            [params setValue:[UIDevice currentDevice].systemVersion forKey:@"platform_version"];
            [params setValue:[MPIdentityProvider identifier] forKey:@"device_id"];
            [params setValue:[UIDevice currentDevice].model forKey:@"device_model"];
            [params setValue:[NSNumber numberWithInteger:self.adConfiguration.adType] forKey:@"ad_type"];
            [params setValue:self.adUnitId forKey:@"ad_unit_id"];
            [params setValue:self.adConfiguration.dspCreativeId forKey:@"creative_id"];
            [params setValue:self.adConfiguration.networkType forKey:@"network_type"];
            [params setValue:[[NSLocale currentLocale] localeIdentifier] forKey:@"device_locale"];
            [params setValue:[self.location description] forKey:@"location"];
            [params setValue:MP_SDK_VERSION forKey:@"sdk_version"];

            if (self.adConfiguration.hasPreferredSize) {
                [params setValue:NSStringFromCGSize(self.adConfiguration.preferredSize) forKey:@"ad_size"];
            }

            if (dateFormatter == nil) {
                dateFormatter = [[NSDateFormatter alloc] init];
                [dateFormatter setTimeStyle:NSDateFormatterLongStyle];
                [dateFormatter setDateStyle:NSDateFormatterShortStyle];
            }
            [params setValue:[dateFormatter stringFromDate:self.adConfiguration.creationTimestamp] forKey:kTimestampParamKey];

            [self processAdParams:params andScreenshot:image];

            MPLogInfo(@"MPAdAlertManager finished processing ad alert");
        });
    });
}

- (void)handleAdAlertGesture
{
    MPLogInfo(@"MPAdAlertManager alert gesture recognized");

    [self.delegate adAlertManagerDidTriggerAlert:self];
}

- (void)processAdParams:(NSDictionary *)params andScreenshot:(UIImage *)screenshot
{
    NSData *imageData = UIImagePNGRepresentation(screenshot);
    NSData *paramData =[[self stringFromDictionary:params] dataUsingEncoding:NSUTF8StringEncoding];
    NSData *markupData = self.adConfiguration.adResponseData;

    self.currentOpenMailVC = [[MFMailComposeViewController alloc] init];
    self.currentOpenMailVC.mailComposeDelegate = self;

    [self.currentOpenMailVC setToRecipients:[NSArray arrayWithObject:@"creative-review@mopub.com"]];
    [self.currentOpenMailVC setSubject:[NSString stringWithFormat:@"New creative violation report - %@", [params objectForKey:kTimestampParamKey]]];
    [self.currentOpenMailVC setMessageBody:@"" isHTML:YES];

    if (imageData != nil) {
        [self.currentOpenMailVC addAttachmentData:imageData mimeType:@"image/png" fileName:@"mp_adalert_screenshot.png"];
    }

    if (paramData != nil) {
        [self.currentOpenMailVC addAttachmentData:paramData mimeType:@"text/plain" fileName:@"mp_adalert_parameters.txt"];
    }

    if (markupData != nil) {
        [self.currentOpenMailVC addAttachmentData:markupData mimeType:@"text/html" fileName:@"mp_adalert_markup.html"];
    }

    [[self.delegate viewControllerForPresentingMailVC] presentViewController:self.currentOpenMailVC animated:MP_ANIMATED completion:nil];

    if ([self.delegate respondsToSelector:@selector(adAlertManagerDidProcessAlert:)]) {
        [self.delegate adAlertManagerDidProcessAlert:self];
    }
}

// could just use [dictionary description], but this gives us more control over the output
- (NSString *)stringFromDictionary:(NSDictionary *)dictionary
{
    NSMutableString *result = [NSMutableString string];

    for (NSString *key in [dictionary allKeys]) {
        [result appendFormat:@"%@ : %@\n", key, [dictionary objectForKey:key]];
    }

    return result;
}

#pragma mark - <MFMailComposeViewControllerDelegate>

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
    self.currentOpenMailVC = nil;

    // reset processed state to allow the user to alert on this ad again
    if (result == MFMailComposeResultCancelled || result == MFMailComposeResultFailed) {
        self.processedAlert = NO;
    }

    [[self.delegate viewControllerForPresentingMailVC] dismissViewControllerAnimated:MP_ANIMATED completion:nil];
}

#pragma mark - Public

- (void)beginMonitoringAlerts
{
    [self endMonitoringAlerts];

    [self.targetAdView addGestureRecognizer:self.adAlertGestureRecognizer];

    // dynamically set minimum tracking distance to account for all ad sizes
    self.adAlertGestureRecognizer.minTrackedDistanceForZigZag = self.targetAdView.bounds.size.width / 3;

    self.processedAlert = NO;
}

- (void)endMonitoringAlerts
{
    [self.targetAdView removeGestureRecognizer:self.adAlertGestureRecognizer];
}

- (void)processAdAlertOnce
{
    if (self.processedAlert) {
        return;
    }

    self.processedAlert = YES;

    [self processAdAlert];
}

#pragma mark - <UIGestureRecognizerDelegate>

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    if ([touch.view isKindOfClass:[UIButton class]]) {
        // we touched a button
        return NO; // ignore the touch
    }
    return YES; // handle the touch
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer;
{
    return YES;
}

@end
