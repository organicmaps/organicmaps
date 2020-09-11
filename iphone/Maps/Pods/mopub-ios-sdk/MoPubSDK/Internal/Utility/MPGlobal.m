//
//  MPGlobal.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPGlobal.h"
#import "MPConstants.h"
#import "MPLogging.h"
#import "NSURL+MPAdditions.h"
#import "MoPub.h"
#import <CommonCrypto/CommonDigest.h>

#import <sys/types.h>
#import <sys/sysctl.h>

BOOL MPViewHasHiddenAncestor(UIView *view);
UIWindow *MPViewGetParentWindow(UIView *view);
BOOL MPViewIntersectsParentWindow(UIView *view);
NSString *MPSHA1Digest(NSString *string);

UIInterfaceOrientation MPInterfaceOrientation()
{
    return [UIApplication sharedApplication].statusBarOrientation;
}

UIWindow *MPKeyWindow()
{
    return [UIApplication sharedApplication].keyWindow;
}

CGFloat MPStatusBarHeight() {
    if ([UIApplication sharedApplication].statusBarHidden) return 0.0f;

    CGFloat width = CGRectGetWidth([UIApplication sharedApplication].statusBarFrame);
    CGFloat height = CGRectGetHeight([UIApplication sharedApplication].statusBarFrame);

    return (width < height) ? width : height;
}

CGRect MPApplicationFrame(BOOL includeSafeAreaInsets)
{
    // Starting with iOS8, the orientation of the device is taken into account when
    // requesting the key window's bounds. We are making the assumption that the
    // key window is equivalent to the application frame.
    CGRect frame = [UIApplication sharedApplication].keyWindow.frame;

    if (@available(iOS 11, *)) {
        if (includeSafeAreaInsets) {
            // Safe area insets include the status bar offset.
            UIEdgeInsets safeInsets = UIApplication.sharedApplication.keyWindow.safeAreaInsets;
            frame.origin.x = safeInsets.left;
            frame.size.width -= (safeInsets.left + safeInsets.right);
            frame.origin.y = safeInsets.top;
            frame.size.height -= (safeInsets.top + safeInsets.bottom);

            return frame;
        }
    }

    frame.origin.y += MPStatusBarHeight();
    frame.size.height -= MPStatusBarHeight();

    return frame;
}

CGRect MPScreenBounds()
{
    // Starting with iOS8, the orientation of the device is taken into account when
    // requesting the key window's bounds.
    return [UIScreen mainScreen].bounds;
}

CGSize MPScreenResolution()
{
    CGRect bounds = MPScreenBounds();
    CGFloat scale = MPDeviceScaleFactor();

    return CGSizeMake(bounds.size.width*scale, bounds.size.height*scale);
}

CGFloat MPDeviceScaleFactor()
{
    if ([[UIScreen mainScreen] respondsToSelector:@selector(displayLinkWithTarget:selector:)] &&
        [[UIScreen mainScreen] respondsToSelector:@selector(scale)]) {
        return [[UIScreen mainScreen] scale];
    } else {
        return 1.0;
    }
}

NSDictionary *MPDictionaryFromQueryString(NSString *query) {
    NSMutableDictionary *queryDict = [NSMutableDictionary dictionary];
    NSArray *queryElements = [query componentsSeparatedByString:@"&"];
    for (NSString *element in queryElements) {
        NSArray *keyVal = [element componentsSeparatedByString:@"="];
        NSString *key = [keyVal objectAtIndex:0];
        NSString *value = [keyVal lastObject];
        [queryDict setObject:[value stringByRemovingPercentEncoding] forKey:key];
    }
    return queryDict;
}

NSString *MPSHA1Digest(NSString *string)
{
    unsigned char digest[CC_SHA1_DIGEST_LENGTH];
    NSData *data = [string dataUsingEncoding:NSASCIIStringEncoding];
    CC_SHA1([data bytes], (CC_LONG)[data length], digest);

    NSMutableString *output = [NSMutableString stringWithCapacity:CC_SHA1_DIGEST_LENGTH * 2];
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++) {
        [output appendFormat:@"%02x", digest[i]];
    }

    return output;
}

BOOL MPViewIsVisible(UIView *view)
{
    // In order for a view to be visible, it:
    // 1) must not be hidden,
    // 2) must not have an ancestor that is hidden,
    // 3) must be within the frame of its parent window.
    //
    // Note: this function does not check whether any part of the view is obscured by another view.

    return (!view.hidden &&
            !MPViewHasHiddenAncestor(view) &&
            MPViewIntersectsParentWindow(view));
}

BOOL MPViewHasHiddenAncestor(UIView *view)
{
    UIView *ancestor = view.superview;
    while (ancestor) {
        if (ancestor.hidden) return YES;
        ancestor = ancestor.superview;
    }
    return NO;
}

UIWindow *MPViewGetParentWindow(UIView *view)
{
    UIView *ancestor = view.superview;
    while (ancestor) {
        if ([ancestor isKindOfClass:[UIWindow class]]) {
            return (UIWindow *)ancestor;
        }
        ancestor = ancestor.superview;
    }
    return nil;
}

BOOL MPViewIntersectsParentWindow(UIView *view)
{
    UIWindow *parentWindow = MPViewGetParentWindow(view);

    if (parentWindow == nil) {
        return NO;
    }

    // We need to call convertRect:toView: on this view's superview rather than on this view itself.
    CGRect viewFrameInWindowCoordinates = [view.superview convertRect:view.frame toView:parentWindow];

    return CGRectIntersectsRect(viewFrameInWindowCoordinates, parentWindow.frame);
}

BOOL MPViewIntersectsParentWindowWithPercent(UIView *view, CGFloat percentVisible)
{
    UIWindow *parentWindow = MPViewGetParentWindow(view);

    if (parentWindow == nil) {
        return NO;
    }

    // We need to call convertRect:toView: on this view's superview rather than on this view itself.
    CGRect viewFrameInWindowCoordinates = [view.superview convertRect:view.frame toView:parentWindow];
    CGRect intersection = CGRectIntersection(viewFrameInWindowCoordinates, parentWindow.frame);

    CGFloat intersectionArea = CGRectGetWidth(intersection) * CGRectGetHeight(intersection);
    CGFloat originalArea = CGRectGetWidth(view.bounds) * CGRectGetHeight(view.bounds);

    return intersectionArea >= (originalArea * percentVisible);
}

NSString *MPResourcePathForResource(NSString *resourceName)
{
    if ([[NSBundle mainBundle] pathForResource:@"MoPub" ofType:@"bundle"] != nil) {
        return [@"MoPub.bundle" stringByAppendingPathComponent:resourceName];
    }
    else {
        // When using open source or cocoapods (on ios 8 and above), we can rely on the MoPub class
        // living in the same bundle/framework as the assets.
        // We can use pathForResource on ios 8 and above to succesfully load resources.
        NSBundle *resourceBundle = [NSBundle bundleForClass:[MoPub class]];
        NSString *resourcePath = [resourceBundle pathForResource:resourceName ofType:nil];
        return resourcePath;
    }
}

NSArray *MPConvertStringArrayToURLArray(NSArray *strArray)
{
    NSMutableArray *urls = [NSMutableArray array];

    for (NSObject *str in strArray) {
        if ([str isKindOfClass:[NSString class]]) {
            NSURL *url = [NSURL URLWithString:(NSString *)str];
            if (url) {
                [urls addObject:url];
            }
        }
    }

    return urls;
}

UIInterfaceOrientationMask MPInterstitialOrientationTypeToUIInterfaceOrientationMask(MPInterstitialOrientationType type)
{
    switch (type) {
        case MPInterstitialOrientationTypePortrait: return UIInterfaceOrientationMaskPortrait;
        case MPInterstitialOrientationTypeLandscape: return UIInterfaceOrientationMaskLandscape;
        default: return UIInterfaceOrientationMaskAll;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation UIDevice (MPAdditions)

- (NSString *)mp_hardwareDeviceName
{
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString *platform = [NSString stringWithCString:machine encoding:NSUTF8StringEncoding];
    free(machine);
    return platform;
}

@end

@implementation UIApplication (MPAdditions)

- (BOOL)mp_supportsOrientationMask:(UIInterfaceOrientationMask)orientationMask
{
    NSArray *supportedOrientations = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"UISupportedInterfaceOrientations"];

    if (orientationMask & UIInterfaceOrientationMaskLandscapeLeft) {
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationLandscapeLeft"]) {
            return YES;
        }
    }

    if (orientationMask & UIInterfaceOrientationMaskLandscapeRight) {
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationLandscapeRight"]) {
            return YES;
        }
    }

    if (orientationMask & UIInterfaceOrientationMaskPortrait) {
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationPortrait"]) {
            return YES;
        }
    }

    if (orientationMask & UIInterfaceOrientationMaskPortraitUpsideDown) {
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationPortraitUpsideDown"]) {
            return YES;
        }
    }

    return NO;
}

- (BOOL)mp_doesOrientation:(UIInterfaceOrientation)orientation matchOrientationMask:(UIInterfaceOrientationMask)orientationMask
{
    BOOL supportsLandscapeLeft = (orientationMask & UIInterfaceOrientationMaskLandscapeLeft) > 0;
    BOOL supportsLandscapeRight = (orientationMask & UIInterfaceOrientationMaskLandscapeRight) > 0;
    BOOL supportsPortrait = (orientationMask & UIInterfaceOrientationMaskPortrait) > 0;
    BOOL supportsPortraitUpsideDown = (orientationMask & UIInterfaceOrientationMaskPortraitUpsideDown) > 0;

    if (supportsLandscapeLeft && orientation == UIInterfaceOrientationLandscapeLeft) {
        return YES;
    }

    if (supportsLandscapeRight && orientation == UIInterfaceOrientationLandscapeRight) {
        return YES;
    }

    if (supportsPortrait && orientation == UIInterfaceOrientationPortrait) {
        return YES;
    }

    if (supportsPortraitUpsideDown && orientation == UIInterfaceOrientationPortraitUpsideDown) {
        return YES;
    }

    return NO;
}

@end
