//
//  MPActivityViewControllerHelper.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPActivityViewControllerHelper.h"

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

@interface MPActivityViewControllerHelper()

- (UIActivityViewController *)initializeActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body;

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

- (UIActivityViewController *)initializeActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body
{
    if (NSClassFromString(@"UIActivityViewController") && NSClassFromString(@"UIActivityItemProvider")) {
        MPActivityItemProviderWithSubject *activityItemProvider =
            [[MPActivityItemProviderWithSubject alloc] initWithSubject:subject body:body];
        UIActivityViewController *activityViewController =
            [[UIActivityViewController alloc] initWithActivityItems:@[activityItemProvider] applicationActivities:nil];
        activityViewController.completionWithItemsHandler = ^(UIActivityType  _Nullable activityType, BOOL completed, NSArray * _Nullable returnedItems, NSError * _Nullable activityError) {
            if ([self.delegate respondsToSelector:@selector(activityViewControllerDidDismiss)]) {
                [self.delegate activityViewControllerDidDismiss];
            }
        };
        return activityViewController;
    } else {
        return nil;
    }
}

- (BOOL)presentActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body
{
    if (NSClassFromString(@"UIActivityViewController")) {
        UIActivityViewController *activityViewController = [self initializeActivityViewControllerWithSubject:subject body:body];
        if (activityViewController) {
            if ([self.delegate respondsToSelector:@selector(activityViewControllerWillPresent)]) {
                [self.delegate activityViewControllerWillPresent];
            }

            UIUserInterfaceIdiom userInterfaceIdiom = UIDevice.currentDevice.userInterfaceIdiom;
            // iPad must present as popover on iOS >= 8
            if (userInterfaceIdiom == UIUserInterfaceIdiomPad) {
                if ([activityViewController respondsToSelector:@selector(popoverPresentationController)]) {
                    activityViewController.popoverPresentationController.sourceView =
                        [self.delegate viewControllerForPresentingActivityViewController].view;
                }
            }

            UIViewController *viewController = [self.delegate viewControllerForPresentingActivityViewController];
            [viewController presentViewController:activityViewController
                                         animated:YES
                                       completion:nil];
            return YES;
        }
    }

    return NO;
}

@end
