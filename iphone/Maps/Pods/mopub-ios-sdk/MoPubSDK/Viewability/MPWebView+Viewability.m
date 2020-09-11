//
//  MPWebView+Viewability.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPWebView+Viewability.h"
#import <WebKit/WebKit.h>

@interface MPWebView ()

- (WKWebView *)wkWebView;

@end

@implementation MPWebView (Viewability)

- (UIView *)containedWebView {
    return self.wkWebView;
}

@end
