//
//  MPAnalyticsTracker.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPAdConfiguration;
@class MPVASTTrackingEvent;

@protocol MPAnalyticsTracker <NSObject>

- (void)trackImpressionForConfiguration:(MPAdConfiguration *)configuration;
- (void)trackClickForConfiguration:(MPAdConfiguration *)configuration;
- (void)sendTrackingRequestForURLs:(NSArray<NSURL *> *)URLs;

@end

@interface MPAnalyticsTracker : NSObject

+ (MPAnalyticsTracker *)sharedTracker;

@end

@interface MPAnalyticsTracker (MPAnalyticsTracker) <MPAnalyticsTracker>
@end
