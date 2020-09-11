//
//  MPDiskLRUCache.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPMediaFileCache.h"

@protocol MPDiskLRUCache <NSObject>

/*
 * Do NOT call any of the following methods on the main thread, potentially lengthy wait for disk IO
 */
- (BOOL)cachedDataExistsForKey:(NSString *)key;
- (NSData *)retrieveDataForKey:(NSString *)key;
- (void)storeData:(NSData *)data forKey:(NSString *)key;
- (void)removeAllCachedFiles;

@end

@interface MPDiskLRUCache : NSObject

+ (MPDiskLRUCache *)sharedDiskCache;

@end

@interface MPDiskLRUCache (MPDiskLRUCache) <MPDiskLRUCache>
@end

@interface MPDiskLRUCache (MPMediaFileCache) <MPMediaFileCache>
@end
