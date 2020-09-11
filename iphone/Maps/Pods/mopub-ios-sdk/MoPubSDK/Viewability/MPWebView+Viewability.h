//
//  MPWebView+Viewability.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPWebView.h"

@interface MPWebView (Viewability)

/**
 * Returns the @c WKWebView instance attached to this @c MPWebView. Exposed for the purpose of having
 * a reliable way to attach viewability SDKs to a web view.
 *
 * Note: Please do not alter the hierarchy of this view (i.e., don't ever call it with `addSubview` or
 * `removeFromSuperview`). Call those methods on the MPWebView instance instead.
 */
@property (nonatomic, readonly) UIView *containedWebView;

@end
