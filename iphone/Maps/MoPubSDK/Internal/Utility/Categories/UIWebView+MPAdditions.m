//
//  UIWebView+MPAdditions.m
//  MoPub
//
//  Created by Andrew He on 11/6/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import "UIWebView+MPAdditions.h"

NSString *const kJavaScriptDisableDialogSnippet = @"window.alert = function() { }; window.prompt = function() { }; window.confirm = function() { };";

@implementation UIWebView (MPAdditions)

/*
 * Find all subviews that are UIScrollViews or subclasses and set their scrolling and bounce.
 */
- (void)mp_setScrollable:(BOOL)scrollable {
    #if __IPHONE_OS_VERSION_MAX_ALLOWED >= 50000 // iOS 5.0+
    if ([self respondsToSelector:@selector(scrollView)])
    {
        UIScrollView *scrollView = self.scrollView;
        scrollView.scrollEnabled = scrollable;
        scrollView.bounces = scrollable;
    }
    else
    #endif
    {
        UIScrollView *scrollView = nil;
        for (UIView *v in self.subviews)
        {
            if ([v isKindOfClass:[UIScrollView class]])
            {
                scrollView = (UIScrollView *)v;
                break;
            }
        }
        scrollView.scrollEnabled = scrollable;
        scrollView.bounces = scrollable;
    }
}

/*
 * Redefine alert, prompt, and confirm to do nothing
 */
- (void)disableJavaScriptDialogs
{
    [self stringByEvaluatingJavaScriptFromString:kJavaScriptDisableDialogSnippet];
}

@end
