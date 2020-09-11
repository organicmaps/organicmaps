//
//  MPEnhancedDeeplinkRequest.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface MPEnhancedDeeplinkRequest : NSObject

@property (copy) NSURL *originalURL;

// Request components derived from the original URL.
@property (copy) NSURL *primaryURL;
@property (strong) NSArray *primaryTrackingURLs;
@property (copy) NSURL *fallbackURL;
@property (strong) NSArray *fallbackTrackingURLs;

- (instancetype)initWithURL:(NSURL *)URL;

@end
