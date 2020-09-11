//
//  MPMemoryCache.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

/**
 Thread-safe memory cache. Items contained within the cache may be evicted
 at the operating system's discretion.
 */
@interface MPMemoryCache : NSObject

/**
 Singleton instance.
 */
+ (instancetype _Nonnull)sharedInstance;

/**
 Retrieves the cached data for the given key, if it exists.
 @param key Key into the cache
 @return The cached data if it exists; otherwise @c nil.
 */
- (NSData * _Nullable)dataForKey:(NSString * _Nonnull)key;

/**
 Sets the cache entry for the given key. If a value of @c nil is given
 as the data, the cache entry will be cleared.
 @param data New data for the cache entry. A value of @c nil will clear the cache entry.
 @param key Key into the cache
 */
- (void)setData:(NSData * _Nullable)data forKey:(NSString * _Nonnull)key;

@end

@interface MPMemoryCache (UIImage)

/**
 Retrieves the cached data as a @c UIImage for the given key, if it exists.
 @param key Key into the cache
 @return The cached image if it exists and contains image data; otherwise @c nil.
 */
- (UIImage * _Nullable)imageForKey:(NSString * _Nonnull)key;

@end
