//
//  MPActivityViewControllerHelper.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPActivityViewControllerHelper.h"
#import "MPInstanceProvider.h"


#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
/**
 * MPActivityItemProviderWithSubject subclasses UIActivityItemProvider
 * to provide a subject for email activity types.
 */

@interface MPActivityItemProviderWithSubject : UIActivityItemProvider

@property (nonatomic, readonly) NSString *subject;
@property (nonatomic, readonly) NSString *body;

- (instancetype)initWithSubject:(NSString *)subject body:(NSString *)body;

@end

@implementation MPActivityItemProviderWithSubject

- (instancetype)initWithSubject:(NSString *)subject body:(NSString *)body
{
    self = [super initWithPlaceholderItem:body];
    if (self) {
        _subject = [subject copy];
        _body = [body copy];
    }
    return self;
}

- (id)item
{
    return self.body;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController subjectForActivityType:(NSString *)activityType
{
    return self.subject;
}

@end
#endif

@interface MPActivityViewControllerHelper()

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (UIActivityViewController *)initializeActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body;
#endif

@end

@implementation MPActivityViewControllerHelper

- (instancetype)initWithDelegate:(id<MPActivityViewControllerHelperDelegate>)delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;
    }
    return self;
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (UIActivityViewController *)initializeActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body
{
    if (NSClassFromString(@"UIActivityViewController") && NSClassFromString(@"UIActivityItemProvider")) {
        MPActivityItemProviderWithSubject *activityItemProvider =
            [[MPActivityItemProviderWithSubject alloc] initWithSubject:subject body:body];
        UIActivityViewController *activityViewController =
            [[UIActivityViewController alloc] initWithActivityItems:@[activityItemProvider] applicationActivities:nil];
        activityViewController.completionHandler = ^
            (NSString* activityType, BOOL completed) {
                if ([self.delegate respondsToSelector:@selector(activityViewControllerDidDismiss)]) {
                    [self.delegate activityViewControllerDidDismiss];
                }
            };
        return activityViewController;
    } else {
        return nil;
    }
}
#endif

- (BOOL)presentActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
    if (NSClassFromString(@"UIActivityViewController")) {
        UIActivityViewController *activityViewController = [self initializeActivityViewControllerWithSubject:subject body:body];
        if (activityViewController) {
            if ([self.delegate respondsToSelector:@selector(activityViewControllerWillPresent)]) {
                [self.delegate activityViewControllerWillPresent];
            }
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 80000
            UIUserInterfaceIdiom userInterfaceIdiom = [[[MPCoreInstanceProvider sharedProvider]
                                                        sharedCurrentDevice] userInterfaceIdiom];
            // iPad must present as popover on iOS >= 8
            if (userInterfaceIdiom == UIUserInterfaceIdiomPad) {
                if ([activityViewController respondsToSelector:@selector(popoverPresentationController)]) {
                    activityViewController.popoverPresentationController.sourceView =
                        [self.delegate viewControllerForPresentingActivityViewController].view;
                }
            }
#endif
            UIViewController *viewController = [self.delegate viewControllerForPresentingActivityViewController];
            [viewController presentViewController:activityViewController
                                         animated:YES
                                       completion:nil];
            return YES;
        }
    }
#endif
    return NO;
}

@end
