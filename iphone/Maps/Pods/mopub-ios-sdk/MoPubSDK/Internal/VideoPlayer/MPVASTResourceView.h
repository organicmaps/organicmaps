//
//  MPVASTResourceView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTResource.h"
#import "MPWebView.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MPVASTResourceViewEvent) {
    MPVASTResourceViewEvent_ClickThrough, // this is a VAST click-through, not a web view click-through
    MPVASTResourceViewEvent_DidLoadView,
    MPVASTResourceViewEvent_FailedToLoadView
};

@class MPVASTResourceView;

@protocol MPVASTResourceViewDelegate <NSObject>

- (void)vastResourceView:(MPVASTResourceView *)vastResourceView
         didTriggerEvent:(MPVASTResourceViewEvent)event;

/**
 This web view click-through is different from the VAST version (`MPVASTResourceViewEvent_ClickThrough`).
 VAST click-through is triggered by tapping on a static image resource and uses the click-through
 URL in the VAST XML, while this web view click-through is triggered by tapping on any link in the
 VAST resource creative.
 */
- (void)vastResourceView:(MPVASTResourceView *)vastResourceView
didTriggerOverridingClickThrough:(NSURL *)url;

@end

@interface MPVASTResourceView : MPWebView

@property (nonatomic, readonly) BOOL isLoaded;
@property (nonatomic, weak) id<MPVASTResourceViewDelegate> resourceViewDelegate; // not `MPWebView.delegate`

- (void)loadResource:(MPVASTResource *)resource containerSize:(CGSize)containerSize;

@end

NS_ASSUME_NONNULL_END
