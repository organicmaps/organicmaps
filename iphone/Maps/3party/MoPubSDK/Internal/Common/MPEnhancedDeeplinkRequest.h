//
//  MPEnhancedDeeplinkRequest.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
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
