//
//  MPVASTResourceView.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTResourceView.h"

/*
 The HTML format strings are inspired by VastResource.java of the MoPub Android SDK. The web content
 of all kinds should not scrollable nor scalable. `WKWebView` might not set the initial scale as 1.0,
 thus we need to explicitly specify it in the `meta` tag.
 Note: One "pixel" in `WKWebView` is actually one "point".
 Warning: As a format string, remember to escape the "%" character as "%%", not "\%". Otherwise,
 100% will be recognized as 100px.
 */
@interface MPVASTResourceView ()

@property (nonatomic, strong) UITapGestureRecognizer *tapGestureRecognizer;

@end

@interface MPVASTResourceView (MPWebViewDelegate) <MPWebViewDelegate>
@end

@interface MPVASTResourceView (UIGestureRecognizerDelegate) <UIGestureRecognizerDelegate>
@end

@implementation MPVASTResourceView

- (instancetype)init {
    self = [super initWithFrame:CGRectZero];
    if (self) {
        self.delegate = self;
        self.backgroundColor = UIColor.blackColor;
    }
    return self;
}

- (void)loadResource:(MPVASTResource *)resource containerSize:(CGSize)containerSize {
    if (resource == nil) {
        [self.resourceViewDelegate vastResourceView:self didTriggerEvent:MPVASTResourceViewEvent_FailedToLoadView];
        return;
    }

    // For static image resource, add a tap gesture recognizer to handle click-through. For all
    // other resource types, let the web view navigtion delegate handle the click-through.
    if (resource.type == MPVASTResourceType_StaticImage) {
        [self addTapGestureRecognizer];
    } else {
        [self removeTapGestureRecognizer]; // in case of the view is reused
    }

    NSString *htmlString = [MPVASTResource fullHTMLRespresentationForContent:resource.content
                                                                        type:resource.type
                                                               containerSize:containerSize];
    [self loadHTMLString:htmlString baseURL:nil];
}

#pragma mark - Private

- (void)addTapGestureRecognizer {
    if (self.tapGestureRecognizer == nil) {
        self.tapGestureRecognizer = [[UITapGestureRecognizer alloc]
                                     initWithTarget:self
                                     action:@selector(handleClickThrough)];
        self.tapGestureRecognizer.delegate = self;
        [self addGestureRecognizer:self.tapGestureRecognizer];
    }
}

- (void)removeTapGestureRecognizer {
    if (self.tapGestureRecognizer != nil) {
        [self removeGestureRecognizer:self.tapGestureRecognizer];
        self.tapGestureRecognizer = nil;
    }
}

- (void)handleClickThrough {
    [self.resourceViewDelegate vastResourceView:self didTriggerEvent:MPVASTResourceViewEvent_ClickThrough];
}

@end

#pragma mark - MPWebViewDelegate

@implementation MPVASTResourceView (MPWebViewDelegate)

- (BOOL)webView:(MPWebView *)webView
shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(WKNavigationType)navigationType {
    if ([request.URL.absoluteString isEqualToString:@"about:blank"]) {
        return YES; // `WKWebView` always starts with a blank page
    } else {
        [self.resourceViewDelegate vastResourceView:self didTriggerOverridingClickThrough:request.URL];
        return NO; // always delegate click-through handling instead of handling here
    }
}

- (void)webViewDidFinishLoad:(MPWebView *)webView {
    _isLoaded = YES;
    [self.resourceViewDelegate vastResourceView:self didTriggerEvent:MPVASTResourceViewEvent_DidLoadView];
}

- (void)webView:(MPWebView *)webView didFailLoadWithError:(NSError *)error {
    [self.resourceViewDelegate vastResourceView:self didTriggerEvent:MPVASTResourceViewEvent_FailedToLoadView];
}

@end

#pragma mark - UIGestureRecognizerDelegate

@implementation MPVASTResourceView (UIGestureRecognizerDelegate)

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES; // need this for the web view to recognize the tap
}

@end
