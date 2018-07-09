//
//  MPWebView+Viewability.m
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPWebView+Viewability.h"
#import <WebKit/WebKit.h>

@interface MPWebView ()

- (UIWebView *)uiWebView;
- (WKWebView *)wkWebView;

@end

@implementation MPWebView (Viewability)

- (UIView *)containedWebView {
    return self.uiWebView ? self.uiWebView : self.wkWebView;
}

@end
