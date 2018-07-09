//
//  NSHTTPURLResponse+MPAdditions.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const kMoPubHTTPHeaderContentType;

@interface NSHTTPURLResponse (MPAdditions)

- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType;

@end
